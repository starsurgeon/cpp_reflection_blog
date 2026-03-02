## Uses global settings from the repository root CMake (C++26 + reflection flags).
## This file only defines the cli_parser example target.

add_reflected_executable(cli_parser src/cli_parser.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Prefer the dev toolchain's libc++ headers if available.
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_include_directories(cli_parser PRIVATE "/opt/clang-p2996/include/c++/v1")
    target_include_directories(cli_parser PRIVATE "/opt/clang-p2996/include/x86_64-unknown-linux-gnu/c++/v1")
  endif()

  # Avoid mixing system libstdc++ headers with libc++ headers.
  if(EXISTS "/opt/clang-p2996/include/c++/v1")
    target_compile_options(cli_parser PRIVATE -nostdinc++)
  endif()

  # Link and runtime search path for the bundled libc++.
  if(EXISTS "/opt/clang-p2996/lib/x86_64-unknown-linux-gnu")
    target_link_options(cli_parser PRIVATE
      "-L/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-Wl,-rpath,/opt/clang-p2996/lib/x86_64-unknown-linux-gnu"
      "-lc++" "-lc++abi")
  elseif(EXISTS "/opt/clang-p2996/lib")
    target_link_options(cli_parser PRIVATE
      "-L/opt/clang-p2996/lib"
      "-Wl,-rpath,/opt/clang-p2996/lib"
      "-lc++" "-lc++abi")
  endif()
endif()

install(TARGETS cli_parser RUNTIME DESTINATION bin)
