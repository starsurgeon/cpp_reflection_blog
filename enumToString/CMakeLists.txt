# Use the helper from the root CMake to create the executable and
# ensure the reflection compile flag and C++ standard are applied.
add_reflected_executable(enumToString src/simple_enum_to_string.cpp)
add_reflected_executable(enumToStringv2 src/advanced_enum_to_string.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # prefer the devtoolchain's libc++ headers if available
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_include_directories(enumToString PRIVATE "/opt/clang-p2996/include/c++/v1")
    target_include_directories(enumToString PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
    target_include_directories(enumToStringv2 PRIVATE "/opt/clang-p2996/include/c++/v1")
    target_include_directories(enumToStringv2 PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
  endif()

  # Prefer the bundled libc++ implementation and avoid picking up the
  # system libstdc++ headers which would conflict with libc++ headers.
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_compile_options(enumToString PRIVATE -nostdinc++)
    target_compile_options(enumToStringv2 PRIVATE -nostdinc++)
  endif()

  # add runtime/linker flags for the toolchain libc++ location
  if(EXISTS "/opt/clang-p2996/lib/x86_64-unknown-linux-gnu")
    target_link_options(enumToString PRIVATE
      "-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
    target_link_options(enumToStringv2 PRIVATE
      "-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
  elseif(EXISTS "/opt/clang-p2996/lib")
    target_link_options(enumToString PRIVATE
      "-L/opt/clang-p2996/lib"
      "-Wl,-rpath,/opt/clang-p2996/lib"
      "-lc++" "-lc++abi")
    target_link_options(enumToStringv2 PRIVATE
      "-L/opt/clang-p2996/lib"
      "-Wl,-rpath,/opt/clang-p2996/lib"
      "-lc++" "-lc++abi")
  endif()
endif()

install(TARGETS enumToString RUNTIME DESTINATION bin)
install(TARGETS enumToStringv2 RUNTIME DESTINATION bin)
