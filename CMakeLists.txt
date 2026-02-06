cmake_minimum_required(VERSION 3.20)
project(cpp_reflection_blog LANGUAGES CXX)

# Global C++ language standard for all example apps
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd / language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for clangd" FORCE)

# Create a symlink from the build `compile_commands.json` to the source root
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/compile_commands.json")
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
  )
endif()

# When using Clang, enable the experimental Reflection flag globally so
# subprojects inherit it by default.
# We also need to enable the -Wno-c23-extensions flag to suppress warnings about C23 extensions, notably #embed
# In addition we need to enable support for 'template for' by setting the -fexpansion-statements flag
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-freflection-latest -fexpansion-statements -Wno-c23-extensions)
endif()

# Convenience function for adding an executable that requires reflection.
function(add_reflected_executable target)
  add_executable(${target} ${ARGN})
  set_target_properties(${target} PROPERTIES CXX_STANDARD 26 CXX_EXTENSIONS OFF)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE -freflection-latest -fexpansion-statements -Wno-c23-extensions)
  endif()
endfunction()

# Add example subprojects here. Add new application directories with
# their own CMakeLists.txt and then add_subdirectory(<name>).
add_subdirectory(enumToString)
add_subdirectory(jsonserializer)
#add_subdirectory(test)
