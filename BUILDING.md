# Build guide

Use this page when the quick start in `README.md` is not enough. All
commands are run from the repository root unless stated
otherwise.

## Linux quick start

### 1. Install Ubuntu build dependencies

```sh
sudo apt-get update
sudo apt-get install -y clang cmake ninja-build \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev \
  libxinerama-dev
```

The verified build uses X11. A Wayland desktop runs it through XWayland; do not
install Wayland development packages for this configuration.

### 2. Configure, build, and test

```sh
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug --parallel
ctest --preset linux-clang-debug --output-on-failure
```

The first configure downloads pinned Raylib and GLM source revisions if they
are not already available.

### 3. Run

```sh
cd build/linux-clang-debug
./nivlr
```

Run the executable from its own build directory. The game currently uses paths
relative to that directory for resources and player data.

## Build-directory layout

CMake presets create one disposable directory per configuration:

```text
build/
├── dependency-cache/        pinned sources retained for offline rebuilds
├── packages/                extracted ready-to-run preview packages
├── linux-clang-debug/       normal development build
├── linux-clang-release/     Linux package build
├── linux-clang-sanitized/   memory/undefined-behavior checks
└── linux-gcc-debug/         independent GCC build
```

Only directories for presets you have configured will exist. Every preset
directory is generated and safe to remove after backing up saves created there.
`build/dependency-cache/` is also deletable, but retaining it avoids downloading
the pinned dependencies again. `build/packages/` contains extracted player
builds and must not be confused with compiler output.

Do not run `cmake .` and do not create files directly under `build/`.

## Other Linux configurations

| Purpose | Preset |
| --- | --- |
| Normal development | `linux-clang-debug` |
| Release/package build | `linux-clang-release` |
| GCC cross-check | `linux-gcc-debug` |
| AddressSanitizer and UBSan | `linux-clang-sanitized` |

Use the same three-command pattern with any preset:

```sh
cmake --preset PRESET
cmake --build --preset PRESET --parallel
ctest --preset PRESET --output-on-failure
```

In a ptrace-restricted container only, LeakSanitizer may need:

```sh
ASAN_OPTIONS=detect_leaks=0 ctest --preset linux-clang-sanitized --output-on-failure
```

CI does not disable leak detection.

## Create the Linux release package

```sh
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release --parallel
ctest --preset linux-clang-release --output-on-failure
cpack --config build/linux-clang-release/CPackConfig.cmake
cmake \
  -DPACKAGE="$PWD/build/linux-clang-release/Noctis-IV-OM-linux-x86_64.tar.gz" \
  -P cmake/VerifyLinuxPackage.cmake
```

The archive and its `.sha256` checksum are written to
`build/linux-clang-release/`. Package scope and CI evidence are in
[`LINUX_PACKAGING.md`](docs/porting/LINUX_PACKAGING.md).

## Windows builds

Windows builds can be cross-compiled directly on Linux using MinGW, or built
natively on Windows using Visual Studio 2022 / GitHub Actions:

- `.github/workflows/windows.yml` builds and tests MSVC Debug.
- `.github/workflows/windows-package.yml` builds and verifies the portable
  MSVC Release ZIP.

### Cross-compiling on Linux (MinGW)

To build a standalone portable Windows release from Linux:

```sh
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release --parallel
cpack --config build/windows-mingw-release/CPackConfig.cmake
cmake \
  -DPACKAGE="$PWD/build/windows-mingw-release/Noctis-IV-OM-windows-x86_64.zip" \
  -P cmake/VerifyWindowsPackage.cmake
```

This statically links the GCC/C++ runtime so the resulting `nivlr.exe` runs
without extra DLL dependencies.

### Native Windows build (MSVC)

On a Windows machine with Visual Studio 2022, the Release commands are:

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel 2
ctest --preset windows-msvc-release --output-on-failure `
  -LE "cross_platform_review|requires_graphics"
cpack -C Release --config build/windows-msvc-release/CPackConfig.cmake
```

The hosted runner cannot provide a real OpenGL display. The packaged no-input
graphics check for a normal Windows PC is:

```powershell
.\nivlr.exe --graphical-smoke
```

See [`WINDOWS_PACKAGING.md`](docs/porting/WINDOWS_PACKAGING.md) for package
evidence and [`WINDOWS_MSVC.md`](docs/porting/WINDOWS_MSVC.md) for the
remaining compatibility boundary.

## Web build (WebAssembly)

The browser build uses Emscripten 6.0.10, the same version CI pins in
`.github/workflows/web.yml`. Install it once with the official SDK:

```sh
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
~/emsdk/emsdk install 6.0.10
~/emsdk/emsdk activate 6.0.10
```

Then, in each shell that builds the web target:

```sh
source ~/emsdk/emsdk_env.sh
cmake --preset web-release
cmake --build --preset web-release
python3 -m http.server 8090 --directory build/web-release
```

Open `http://localhost:8090/nivlr.html`. The page must be served over HTTP;
opening the file directly does not work. Deploy the four files `nivlr.html`,
`nivlr.js`, `nivlr.wasm`, and `nivlr.data` together.

How the browser build differs from the desktop one:

- **Main loop:** it links with ASYNCIFY, so the inherited blocking space and
  surface loops yield to the browser once per frame in `swapBuffers`.
- **Graphics:** it targets WebGL2, and the CRT shader compiles as GLSL ES 3.00.
- **Assets:** `res/` and the default catalogs are preloaded into the same
  `/res` and `/defaults` layout the desktop packages use.
- **Player data:** saves, catalogs, the gallery, movies, and settings live in
  `/persistent`, which `web/pre.js` backs with IndexedDB. Data stays in that
  browser profile, and clearing site data erases it.
- **Saving:** the game autosaves about every 30 seconds and when the tab is
  hidden or closed, including the surface position while landed. Escape never
  ends the session, because browsers also use it to release mouse capture and
  leave fullscreen; on a planet it saves in place.

## Portable mode

Release presets (`linux-clang-release`, `windows-mingw-release`, and
`windows-msvc-release`) configure `-DNIVLR_PORTABLE_DEFAULT=ON` by default.
In portable mode, all saves, catalogs, screenshots, movies, and configs are
stored directly within the game directory beside the executable.

To use standard OS user directories instead, launch with `--system-user-data`,
or configure CMake with `-DNIVLR_PORTABLE_DEFAULT=OFF`. You can also force
portable mode on any build using the `--portable` CLI flag or by setting
`NOCTIS_IV_OM_PORTABLE=1`.

## Offline build using the retained cache

After the first dependency download, this workspace keeps verified sources in
`build/dependency-cache/`. Configure without network access using:

```sh
cmake --preset linux-clang-debug \
  -DCPM_BOOTSTRAP_FILE="$PWD/build/dependency-cache/CPM_0.43.1.cmake" \
  -DCPM_raylib_SOURCE="$PWD/build/dependency-cache/raylib-dbc56a87da87d973a9c5baa4e7438a9d20121d28" \
  -DCPM_glm_SOURCE="$PWD/build/dependency-cache/glm-8d1fd52e5ab5590e2c81768ace50c72bae28f2ed"
```

Then use the normal build and test commands. A fresh checkout without this
cache needs network access for the first configure.

Pinned dependency revisions:

- Raylib 6.0: `dbc56a87da87d973a9c5baa4e7438a9d20121d28`
- GLM 1.0.3: `8d1fd52e5ab5590e2c81768ace50c72bae28f2ed`
- CPM 0.43.1 SHA-256:
  `1c40fc102ce9625d7de7eb14f541cab30cc3138dca627f0b0ec40293ce6c2934`

## Diagnostics and focused tests

Headless startup diagnostics:

```sh
./build/linux-clang-debug/nivlr --diagnostics
```

Run every test in the selected configuration:

```sh
ctest --preset linux-clang-debug --output-on-failure
```

Run one test by name:

```sh
ctest --test-dir build/linux-clang-debug -R TEST_NAME --output-on-failure
```

List available tests:

```sh
ctest --test-dir build/linux-clang-debug -N
```

Detailed fixture purpose, provenance, and acceptance evidence belong in
[`docs/porting/`](docs/porting/README.md), not in this build guide.

## Common problems

### `res/supports.nct` is missing

Run `nivlr` from `build/<preset>/`, not from the repository root.

### CMake cannot download CPM, Raylib, or GLM

Restore network access or use the offline-cache command above.

### X11 or OpenGL headers are missing

Install the Ubuntu packages from the first section. Native Wayland development
packages are not part of this build.

### Start completely fresh

Remove only the preset directory you want to rebuild, then repeat configure:

```sh
cmake -E remove_directory build/linux-clang-debug
cmake --preset linux-clang-debug
```

Current builds keep player files in the normal OS user-data location rather
than under `build/`. Older builds used colocated `data/`, `gallery/`, and
`movies/` directories; preserve them before removing an old build and import
them with `--migrate-from`. Run `./nivlr --diagnostics` to see the resolved
locations.

For runtime issues, graphics problems, cockpit power recovery, and control
guidance, consult [`TROUBLESHOOTING.md`](TROUBLESHOOTING.md). For contributor
standards and pull-request procedures, see [`CONTRIBUTING.md`](CONTRIBUTING.md).
