## Serialization library with reflection-based JSON serialization
## Uses the project's root CMake for global settings (C++ standard and reflection flags)

# Create test executable (header-only library, no separate compilation needed)
add_reflected_executable(serialization_tests tests/serialization_tests.cpp)
target_include_directories(serialization_tests PRIVATE include)

add_reflected_executable(support_matrix_tests tests/support_matrix_tests.cpp)
target_include_directories(support_matrix_tests PRIVATE include)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # prefer the devtoolchain's libc++ headers if available
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_include_directories(serialization_tests PRIVATE "/opt/clang-p2996/include/c++/v1")
    target_include_directories(serialization_tests PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
    target_include_directories(support_matrix_tests PRIVATE "/opt/clang-p2996/include/c++/v1")
    target_include_directories(support_matrix_tests PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
  endif()

  # Prefer the bundled libc++ implementation
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_compile_options(serialization_tests PRIVATE -nostdinc++)
    target_compile_options(support_matrix_tests PRIVATE -nostdinc++)
  endif()

  # add runtime/linker flags for the toolchain libc++ location
  if(EXISTS "/opt/clang-p2996/lib/x86_64-unknown-linux-gnu")
    target_link_options(serialization_tests PRIVATE
      "-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
    target_link_options(support_matrix_tests PRIVATE
      "-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
  elseif(EXISTS "/opt/clang-p2996/lib")
    target_link_options(serialization_tests PRIVATE
      "-L/opt/clang-p2996/lib"
      "-Wl,-rpath,/opt/clang-p2996/lib"
      "-lc++" "-lc++abi")
    target_link_options(support_matrix_tests PRIVATE
      "-L/opt/clang-p2996/lib"
      "-Wl,-rpath,/opt/clang-p2996/lib"
      "-lc++" "-lc++abi")
  endif()
endif()

if(BUILD_TESTING)
  add_test(NAME serialization_tests COMMAND serialization_tests)
  set_tests_properties(serialization_tests PROPERTIES LABELS "serialization")
  add_test(NAME support_matrix_tests COMMAND support_matrix_tests)
  set_tests_properties(support_matrix_tests PROPERTIES LABELS "serialization")
endif()
