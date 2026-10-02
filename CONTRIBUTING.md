# Contributing to Noctis IV OM

Thank you for your interest in contributing to Noctis IV OM! This document
provides guidelines and instructions for developers contributing to the codebase.

## 1. Project Mission and Philosophy

Noctis IV OM is a modern C++20 port of Alessandro Ghignola's space exploration
game *Noctis IV*, combining the native C++ foundation of Noctis IV LR with the
features, identity, and content of Noctis IV Plus.

### Guiding Principles

1. **Procedural Determinism is Sacred:** The Noctis universe must remain intact.
   Stars, planetary topologies, names, coordinates, surface features, and
   catalogs must match the established procedural baseline. Never alter PRNG
   sequences, coordinate calculations, or world generators without an explicit,
   proven compatibility rationale backed by regression tests.
2. **Atmosphere and Identity:** The quiet, meditative exploration loop of
   Noctis IV must be preserved. Visual effects, cockpit interfaces, and
   timing models should faithfully honor the original game's feel.
3. **Small Vertical Slices:** Work packages are scoped incrementally according
   to the [Roadmap](docs/porting/ROADMAP.md). Submit small, focused, verifiable
   changes accompanied by tests.
4. **Zero-Regression Gate:** All existing automated tests (unit, fixture,
   journey, determinism, and packaging checks) must pass on every change.

## 2. Development Setup

### Linux (Ubuntu 24.04 or compatible)

Install required system build tools and development libraries:

```sh
sudo apt-get update
sudo apt-get install -y clang cmake ninja-build \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev \
  libxinerama-dev
```

*Note:* Noctis IV OM targets X11. On Wayland desktops, it runs through XWayland.
Do not install Wayland development packages for the native build.

For cross-compiling Windows releases on Linux, also install MinGW:

```sh
sudo apt-get install -y mingw-w64
```

### Windows (Visual Studio 2022)

Install Visual Studio 2022 with the "Desktop development with C++" workload,
including CMake tools for Windows and the MSVC C++ compiler.

## 3. CMake Presets and Build Workflow

The repository uses standard `CMakePresets.json`. All commands are run from the
repository root.

| Preset | Platform / Compiler | Purpose |
| --- | --- | --- |
| `linux-clang-debug` | Linux / Clang | Default development build with debug symbols |
| `linux-clang-release` | Linux / Clang | Release build for benchmarking and packaging |
| `linux-clang-sanitized` | Linux / Clang | AddressSanitizer and UBSan validation |
| `linux-gcc-debug` | Linux / GCC | Secondary compiler validation |
| `windows-mingw-release` | Linux / MinGW | Cross-compiled portable Windows release |
| `windows-msvc-debug` | Windows / MSVC | Windows development build |
| `windows-msvc-release` | Windows / MSVC | Windows portable release package |

### Standard Development Cycle

```sh
# Configure
cmake --preset linux-clang-debug

# Build
cmake --build --preset linux-clang-debug --parallel

# Run all tests
ctest --preset linux-clang-debug --output-on-failure

# Launch the game
./build/linux-clang-debug/nivlr
```

### Sanitizer Verification

Before submitting changes that touch memory management, pointer arithmetic, or
array indexing, run the sanitized preset:

```sh
cmake --build --preset linux-clang-sanitized --parallel
ctest --preset linux-clang-sanitized --output-on-failure
```

This ensures there are no buffer overflows, memory leaks, or undefined behavior.

## 4. Testing Guidelines

Tests live under `tests/` and cover multiple layers:

- **Determinism tests:** Verify PRNG sequences (`brtl_determinism`, `legacy_fast_prng_determinism`).
- **Fixture tests:** Verify star maps, galaxy layouts, surface rendering, and DOS references against golden baselines (`renderer_fixtures`, `dos_*_reference`, `surface_*_fixtures`).
- **Application journeys:** Verify full end-to-end operational flows (`persistence_journey`, `scripted_journey`, `orbit_surface_journey`).
- **Path and packaging tests:** Verify runtime directory resolution and package structure (`runtime_paths`, `VerifyLinuxPackage.cmake`, `VerifyWindowsPackage.cmake`).

### Running Focused Tests

Run a specific test by name or regex:

```sh
ctest --preset linux-clang-debug -R renderer_fixtures --output-on-failure
```

List all available tests:

```sh
ctest --preset linux-clang-debug -N
```

## 5. Code Style and Quality Standards

- **Language Standard:** C++20.
- **Formatting:** Keep code formatted with `clang-format` using the project's `.clang-format` configuration.
- **Warnings as Errors:** The project builds with high warning levels. Fix all compiler warnings.
- **Documentation:** Document public functions, non-obvious algorithms, and procedural constants. Preserve existing comments and docstrings unless explicitly refactoring.
- **Defensive Programming:** Always check array bounds when indexing screen buffers (`adapted`) or world maps.

## 6. Packaging and Verification

To build and verify release archives locally:

### Linux Archive (.tar.gz)

```sh
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release --parallel
ctest --preset linux-clang-release --output-on-failure
cpack --config build/linux-clang-release/CPackConfig.cmake
cmake -DPACKAGE="$PWD/build/linux-clang-release/Noctis-IV-OM-linux-x86_64-preview.tar.gz" \
  -P cmake/VerifyLinuxPackage.cmake
```

### Windows Archive (.zip) via MinGW Cross-Compile

```sh
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release --parallel
cpack --config build/windows-mingw-release/CPackConfig.cmake
cmake -DPACKAGE="$PWD/build/windows-mingw-release/Noctis-IV-OM-windows-x86_64-preview.zip" \
  -P cmake/VerifyWindowsPackage.cmake
```

## 7. Submitting Changes

1. Create a descriptive branch or fork.
2. Keep commits atomic and write clear, informative commit messages explaining *what* was changed and *why*.
3. Ensure all tests pass across Clang, GCC, and Sanitizers.
4. If your change addresses a roadmap work item, update [`docs/porting/ROADMAP.md`](docs/porting/ROADMAP.md) to reflect progress or completion.
5. Open a Pull Request referencing any relevant issues or roadmap IDs.
