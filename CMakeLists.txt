cmake_minimum_required(VERSION 3.20)
project(cpp_reflection_blog LANGUAGES CXX)

include(CTest)

option(CRB_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CRB_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(CRB_ENABLE_COVERAGE "Enable code coverage flags" OFF)
option(CRB_ENABLE_CPPCHECK "Run cppcheck during build" OFF)
option(CRB_ENABLE_IWYU "Run include-what-you-use during build" OFF)
option(CRB_ENABLE_FUZZTEST "Enable fuzz test targets" OFF)
option(CRB_ENABLE_MUTATION "Enable mutation test targets" OFF)
option(CRB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(CRB_WARNINGS_AS_ERRORS)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

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

set(CRB_QUALITY_COMPILE_FLAGS "")
set(CRB_QUALITY_LINK_FLAGS "")
set(CRB_CLANG_RUNTIME_DIR "")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} --print-resource-dir
    OUTPUT_VARIABLE CRB_CLANG_RESOURCE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  file(GLOB CRB_CLANG_RUNTIME_CANDIDATES "${CRB_CLANG_RESOURCE_DIR}/lib/*-linux-gnu")
  list(LENGTH CRB_CLANG_RUNTIME_CANDIDATES CRB_CLANG_RUNTIME_COUNT)
  if(CRB_CLANG_RUNTIME_COUNT GREATER 0)
    list(GET CRB_CLANG_RUNTIME_CANDIDATES 0 CRB_CLANG_RUNTIME_DIR)
  endif()
endif()

if(CRB_ENABLE_ASAN)
  if(NOT CRB_CLANG_RUNTIME_DIR)
    message(FATAL_ERROR "CRB_ENABLE_ASAN is ON but clang runtime directory was not found")
  endif()
  if(NOT EXISTS "${CRB_CLANG_RUNTIME_DIR}/libclang_rt.asan_static.a" OR NOT EXISTS "${CRB_CLANG_RUNTIME_DIR}/libclang_rt.asan-preinit.a")
    message(FATAL_ERROR "CRB_ENABLE_ASAN is ON but required ASan runtime archives are missing in ${CRB_CLANG_RUNTIME_DIR}")
  endif()
  list(APPEND CRB_QUALITY_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
  list(APPEND CRB_QUALITY_LINK_FLAGS -fsanitize=address)
  list(APPEND CRB_QUALITY_LINK_FLAGS "-Wl,-rpath,${CRB_CLANG_RUNTIME_DIR}" "-L${CRB_CLANG_RUNTIME_DIR}")
endif()

if(CRB_ENABLE_UBSAN)
  if(NOT CRB_CLANG_RUNTIME_DIR)
    message(FATAL_ERROR "CRB_ENABLE_UBSAN is ON but clang runtime directory was not found")
  endif()
  if(NOT EXISTS "${CRB_CLANG_RUNTIME_DIR}/libclang_rt.ubsan_standalone.so")
    message(FATAL_ERROR "CRB_ENABLE_UBSAN is ON but libclang_rt.ubsan_standalone.so is missing in ${CRB_CLANG_RUNTIME_DIR}")
  endif()
  list(APPEND CRB_QUALITY_COMPILE_FLAGS -fsanitize=undefined -fno-sanitize-recover=all)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND CRB_QUALITY_COMPILE_FLAGS -shared-libsan)
    list(APPEND CRB_QUALITY_LINK_FLAGS -shared-libsan)
  endif()
  list(APPEND CRB_QUALITY_LINK_FLAGS -fsanitize=undefined)
  list(APPEND CRB_QUALITY_LINK_FLAGS "-Wl,-rpath,${CRB_CLANG_RUNTIME_DIR}" "-L${CRB_CLANG_RUNTIME_DIR}")
endif()

if(CRB_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(NOT CRB_CLANG_RUNTIME_DIR)
      message(FATAL_ERROR "CRB_ENABLE_COVERAGE is ON but clang runtime directory was not found")
    endif()
    if(NOT EXISTS "${CRB_CLANG_RUNTIME_DIR}/libclang_rt.profile.a")
      message(FATAL_ERROR "CRB_ENABLE_COVERAGE is ON but libclang_rt.profile.a is missing in ${CRB_CLANG_RUNTIME_DIR}")
    endif()
  endif()
  list(APPEND CRB_QUALITY_COMPILE_FLAGS -O0 -g -fprofile-arcs -ftest-coverage)
  list(APPEND CRB_QUALITY_LINK_FLAGS -fprofile-arcs -ftest-coverage)
endif()

if(CRB_ENABLE_CPPCHECK)
  find_program(CRB_CPPCHECK_BIN NAMES cppcheck)
  if(CRB_CPPCHECK_BIN)
    set(CMAKE_CXX_CPPCHECK
      ${CRB_CPPCHECK_BIN}
      --enable=warning,style,performance,portability
      --error-exitcode=2
      --inline-suppr
      --quiet
    )
  else()
    message(FATAL_ERROR "CRB_ENABLE_CPPCHECK is ON but cppcheck was not found")
  endif()
endif()

if(CRB_ENABLE_IWYU)
  find_program(CRB_IWYU_BIN NAMES include-what-you-use iwyu)
  if(CRB_IWYU_BIN)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${CRB_IWYU_BIN})
  else()
    message(FATAL_ERROR "CRB_ENABLE_IWYU is ON but include-what-you-use was not found")
  endif()
endif()

if(CRB_ENABLE_MUTATION)
  find_program(CRB_MUTATEPP_BIN NAMES mutate++ mutatepp)
  if(CRB_MUTATEPP_BIN)
    add_custom_target(mutation-smoke
      COMMAND ${CRB_MUTATEPP_BIN} --help
      COMMENT "Running mutate++ smoke check"
      VERBATIM
    )
  else()
    message(FATAL_ERROR "CRB_ENABLE_MUTATION is ON but mutate++ was not found")
  endif()
endif()

function(crb_apply_quality_options target)
  if(CRB_QUALITY_COMPILE_FLAGS)
    target_compile_options(${target} PRIVATE ${CRB_QUALITY_COMPILE_FLAGS})
  endif()
  if(CRB_QUALITY_LINK_FLAGS)
    target_link_options(${target} PRIVATE ${CRB_QUALITY_LINK_FLAGS})
  endif()
endfunction()

# Convenience function for adding an executable that requires reflection.
function(add_reflected_executable target)
  add_executable(${target} ${ARGN})
  set_target_properties(${target} PROPERTIES CXX_STANDARD 26 CXX_EXTENSIONS OFF)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE -freflection-latest -fexpansion-statements -Wno-c23-extensions)
  endif()
  crb_apply_quality_options(${target})
endfunction()

# Add example subprojects here. Add new application directories with
# their own CMakeLists.txt and then add_subdirectory(<name>).
add_subdirectory(enumToString)
add_subdirectory(jsonserializer)
add_subdirectory(std_meta_part2)
add_subdirectory(cli_parser)
add_subdirectory(validation)
#add_subdirectory(test)
