# cli_parser

A small C++ Reflection (P2996) example that maps command-line options to struct members.

Current config fields:
- `--host <value>`
- `--port <value>`
- `--verbose` (presence sets it to `true`)

## Build

From repository root:

```bash
cmake -S . -B build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=/opt/p2996/cmake/p2996-libcxx-toolchain.cmake \
  -DCMAKE_PREFIX_PATH=/opt/qt \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build/Debug --target cli_parser -- -j
```

## Run

```bash
./build/Debug/cli_parser/cli_parser --host example.com --port 9090 --verbose
```

Expected output format:

```text
example.com:9090 verbose=1
```

## Notes

- This target relies on the repository root CMake for global C++26 and reflection flags.
- To include this example in the full workspace build, ensure the root `CMakeLists.txt` contains `add_subdirectory(cli_parser)`.
