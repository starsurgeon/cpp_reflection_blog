## Uses global settings from the repository root CMake (C++26 + reflection flags).
## This file only defines the python_bindings example target.

find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(pybind11 CONFIG QUIET)

set(PYBIND11_FALLBACK_INCLUDE_DIR "")
if(NOT pybind11_FOUND)
  find_path(PYBIND11_FALLBACK_INCLUDE_DIR
    NAMES pybind11/pybind11.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/include
      /opt/local/include)

  if(NOT PYBIND11_FALLBACK_INCLUDE_DIR)
    message(FATAL_ERROR
      "pybind11 headers were not found. Install pybind11 or provide pybind11Config.cmake.")
  endif()
endif()

add_library(python_bindings MODULE src/python_bindings.cpp)
set_target_properties(python_bindings PROPERTIES
  CXX_STANDARD 26
  CXX_EXTENSIONS OFF
  PREFIX ""
  OUTPUT_NAME "example"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python")

if(DEFINED Python3_EXTENSION_MODULE_SUFFIX)
  set_target_properties(python_bindings PROPERTIES SUFFIX "${Python3_EXTENSION_MODULE_SUFFIX}")
endif()

# Dev convenience: keep a copy next to demo_example.py so plain `import example`
# works when running scripts from python_bindings/.
add_custom_command(TARGET python_bindings POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:python_bindings>"
          "${CMAKE_CURRENT_SOURCE_DIR}/$<TARGET_FILE_NAME:python_bindings>")

target_link_libraries(python_bindings PRIVATE Python3::Module)
if(pybind11_FOUND)
  target_link_libraries(python_bindings PRIVATE pybind11::module)
else()
  target_include_directories(python_bindings PRIVATE "${PYBIND11_FALLBACK_INCLUDE_DIR}")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(python_bindings PRIVATE -freflection-latest -fexpansion-statements -Wno-c23-extensions)

  # Prefer the dev toolchain's libc++ headers if available.
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_include_directories(python_bindings PRIVATE "/opt/clang-p2996/include/c++/v1")
    target_include_directories(python_bindings PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
  endif()

  # Avoid mixing system libstdc++ headers with libc++ headers.
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_compile_options(python_bindings PRIVATE -nostdinc++)
  endif()

  # Link and runtime search path for the bundled libc++.
  if(EXISTS "/opt/clang-p2996/lib/x86_64-unknown-linux-gnu")
    target_link_options(python_bindings PRIVATE
      "-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
  elseif(EXISTS "/opt/clang-p2996/lib")
    target_link_options(python_bindings PRIVATE
      "-L/opt/clang-p2996/lib"
      "-Wl,-rpath,/opt/clang-p2996/lib"
      "-lc++" "-lc++abi")
  elseif(EXISTS "/opt/p2996/lib/x86_64-unknown-linux-gnu")
    target_link_options(python_bindings PRIVATE
      "-L/opt/p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
  elseif(EXISTS "/opt/p2996/lib")
    target_link_options(python_bindings PRIVATE
      "-L/opt/p2996/lib"
      "-Wl,-rpath,/opt/p2996/lib"
      "-lc++" "-lc++abi")
  endif()
endif()

install(TARGETS python_bindings
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
