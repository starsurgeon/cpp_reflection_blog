# Examples — C++ reflection blog ✅

This repository now hosts multiple small example applications that exercise the experimental C++ Reflection proposal (P2996) and related tooling. Two example apps are included:

- `enumToString` — a Qt-based example demonstrating enum-to-string and runtime enum metadata.
- `jsonserializer` — a small reflection-driven JSON serializer example.

Important: these examples require an experimental Clang toolchain that implements the C++ Reflection proposal P2996R13. The code relies on reflection features from that proposal and will not compile with standard stable Clang/GCC toolchains. See the proposal: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html

This repository is structured so additional example apps may be added as new subdirectories under the repository root; each app contains its own `CMakeLists.txt` and sources.

---

## Prerequisites 🔧
- CMake >= 3.20
- Ninja (or change generator)
- A Clang toolchain with libc++ and the experimental reflection support (this repository assumes the p2996 toolchain available at `/opt/p2996`)

Notes:
- The examples use a small top-level `CMakeLists.txt` that sets the C++ standard and enables `-freflection-latest` and `-fexpansion-statements` for Clang so subprojects inherit those settings.
- The project uses a toolchain file at `/opt/p2996/cmake/p2996-libcxx-toolchain.cmake` in examples. If you don't have a compatible toolchain installed locally, build the container using the included `Dockerfile` and use the container for building.

---

## Build (recommended — from repository root) 🛠️

Configure the full workspace (creates `build/Debug` configured for the toolchain and Qt):

```bash
cmake -S . -B build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=/opt/p2996/cmake/p2996-libcxx-toolchain.cmake \
  -DCMAKE_PREFIX_PATH=/opt/qt \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Build all example targets:

```bash
cmake --build build/Debug -- -j
```

Or build a single example (e.g. `enumToString` or `jsonparser`):

```bash
cmake --build build/Debug --target enumToString
cmake --build build/Debug --target jsonparser
```

The resulting binaries are placed under `build/Debug/<subdir>/` (for example `build/Debug/enumToString/enumToString`).

---

## VS Code integration (CMake Tools & clangd) 💻
- Use the **CMake Tools** extension and configure the project from the repository root (set `cmake.sourceDirectory` to the repository root or open the repo root in the workspace). This ensures the top-level CMake settings are applied to all subprojects.

Example `.vscode/settings.json` values:

```json
{
  "cmake.sourceDirectory": "${workspaceFolder}",
  "cmake.buildDirectory": "${workspaceFolder}/build/${buildType}",
  "cmake.configureOnOpen": false
}
```

clangd needs the compile_commands.json file to provide accurate diagnostics and includes. Options:

- Point clangd at the workspace build dir that contains `compile_commands.json` (e.g. `build/Debug`).
- Or use the symlink created by the top-level CMake that points `compile_commands.json` into the repository root.

---

## Docker usage 🐳
A Docker image can provide the experimental Clang toolchain and a Qt6 build that links against `libc++`, making builds reproducible without installing special toolchains on the host.

Quick steps:

```bash
# build the image (one-time)
docker build -f Dockerfile -t cpp_reflection_blog:latest .

# run an interactive container with the workspace mounted
docker run --rm -it \
  -v "${PWD}:/workspace:cached" -w /workspace \
  -e DISPLAY="$DISPLAY" -v /tmp/.X11-unix:/tmp/.X11-unix \
  enumtostring-toolchain:latest /bin/bash

# inside the container, configure and build from repo root
cmake -S . -B build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=/opt/p2996/cmake/p2996-libcxx-toolchain.cmake \
  -DCMAKE_PREFIX_PATH=/opt/qt \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/Debug -- -j
```

Notes:
- For GUI apps you may need to allow the container access to your X server (e.g. `xhost +local:root`) or use a Wayland/XWayland setup depending on your host system.
- To integrate with VS Code, add a `.devcontainer/devcontainer.json` referencing the image so the Remote - Containers extension opens the project inside the same environment.

---

## Troubleshooting 🐞
- "fatal error: 'meta' file not found": verify the toolchain file and compile flags are applied (check `compile_commands.json` for `-freflection-latest` and libc++ include paths or `-nostdinc++` + `-I/opt/.../include/c++/v1`).
- If CMake Presets fail with "Invalid macro expansion": use only CMake Preset macros like `${sourceDir}` for `binaryDir` — do not use VS Code-specific macros like `${workspaceFolder}` or unsupported macros like `${buildType}` inside presets. Instead, create explicit presets for Debug/Release.
- If clangd doesn't pick up the compile DB, either point `clangd.arguments` at the right directory or create a symlink at a location clangd checks.

---

## Project structure 📁
```
.
├── CMakeLists.txt         # top-level, adds subdirectories and sets global flags
├── enumToString/          # example app (Qt)
│   ├── CMakeLists.txt
│   └── src/
│       └── simple_enum_to_string.cpp
│       └── advanced_enum_to_string.cpp
├── jsonserializer/        # example app (reflection-driven JSON)
│   ├── CMakeLists.txt
│   └── src/
│       └── jsonserializer.cpp
└── Dockerfile
```

---

## Build Troubleshooting

- Reflection / expansion syntax not recognized (e.g., errors around ^^, [: :], template for)
Your root CMake only adds the required flags when CMAKE_CXX_COMPILER_ID matches Clang. If you accidentally configure with GCC (or a non-Clang ID), those flags won’t be applied. Make sure CMake is actually using your clang-p2996 clang++. 

- “unknown argument: -freflection-latest” / “-fexpansion-statements”
That usually means you’re using a Clang that doesn’t implement these experimental options. Your setup assumes -freflection-latest and -fexpansion-statements exist. 

- Weird standard library errors / “ambiguous” or “redefinition” in <string> / <vector> etc.
The enumToString target intentionally prefers libc++ from /opt/clang-p2996 and adds -nostdinc++ to avoid accidentally mixing libstdc++ headers with libc++ headers. If /opt/clang-p2996/include/c++/v1 doesn’t exist (or you’re not in the container), you can end up with a mismatched stdlib setup. 

- Build succeeds, but running fails (e.g., “libc++.so not found”)
The target adds an rpath pointing at /opt/clang-p2996/lib… and links -lc++ -lc++abi. If your /opt/clang-p2996/lib/... path differs, the binary may not locate the runtime libs. 

- Lots of warnings about C23 extensions (#embed, etc.)
Your root CMake suppresses those with -Wno-c23-extensions (again: only for Clang). If you’re seeing them, you may not be building with Clang or the flags aren’t being applied. 

- compile_commands.json points to the wrong build directory (clangd shows stale errors)
The root CMake creates a symlink from the build tree into the source root only if the file doesn’t already exist. If you switch build dirs, delete the existing compile_commands.json in the source root and reconfigure. 

## License & Contact 📝


Happy hacking! ⚡️
