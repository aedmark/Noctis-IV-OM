# Noctis IV LR Baseline Audit — M0-W02

Audit date: 2026-09-25

Upstream: https://github.com/dgcole/noctis-iv-lr

Audited commit: `e1b0817da580e23062b3d2a64b7b6947bc0cb419`

Upstream commit date: 2026-07-25

Disposition: buildable Linux candidate; final adoption and repository layout are
M0-W03 decisions.

This document records observed facts about one immutable upstream commit. It is
an inventory, not a claim that gameplay or universe compatibility has been
verified. Paths below refer to that upstream commit unless stated otherwise.

## Reproduction

The audit used a clean temporary checkout and an out-of-tree Debug build. On
this host, CMake downloaded dependencies during configuration; network access
and the host's X11 display were needed for the configure and startup checks.

```bash
git clone https://github.com/dgcole/noctis-iv-lr.git /tmp/noctis-iv-lr-audit
git -C /tmp/noctis-iv-lr-audit checkout e1b0817da580e23062b3d2a64b7b6947bc0cb419
cmake -S /tmp/noctis-iv-lr-audit -B /tmp/noctis-iv-lr-build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cmake --build /tmp/noctis-iv-lr-build --parallel 4
ctest --test-dir /tmp/noctis-iv-lr-build -N
```

Results on this host:

- Git 2.55.0, CMake 4.4.3, Clang 22.1.8, x86-64 Linux.
- Configure and compile succeeded without editing upstream files.
- The executable target was `nivlr`; the build created a `res` symlink beside
  it. There is no installation target or package definition in the root CMake
  file.
- The build emitted six first-party Clang warnings: four integer arguments to
  `fabs` in `src/noctis-1.cpp` and two format mismatches on one `sprintf` in
  `src/noctis.cpp`. A separate warning came from Raylib's bundled `stb_vorbis`.
- `ctest -N` reported **0 tests**. No test targets or test files are tracked.
- A five-second startup from the build directory initialized Raylib 6.0, X11,
  OpenGL, and the 320×200 screen texture. The process was stopped by `timeout`
  (exit 124). Gameplay, saving, and shutdown were not validated.
- Startup in the restricted shell could not open display `:0` and aborted after
  GLFW initialization failed. The host-display run above establishes that this
  was a display-access limitation of the audit environment.

### Additional compiler and sanitizer checks

- Passing `/usr/bin/gcc` and `/usr/bin/g++` to CMake did **not** test GCC. The
  generated build still invoked `/usr/bin/clang++`, because the root CMake file
  resets compiler paths after `project()` on Unix. The cache misleadingly kept
  the requested GCC paths.
- Removing only that compiler override in the temporary audit checkout, then
  reconfiguring, produced a real GCC 16.2.1 build. Its generated commands used
  `/usr/bin/g++`, and the build succeeded. This is a test-only change; the
  upstream project at the audited commit still forces Clang.
- A separate Clang build with `-fsanitize=undefined` started on the host display
  and reported four distinct diagnostics within five seconds. These are
  observed issues in active code, not merely static concerns:

  | Location | Diagnostic | Likely area |
  | --- | --- | --- |
  | `src/noctis.cpp:652` | Misaligned 32-bit load | Onboard screen glyph rendering |
  | `src/noctis.cpp:3501` | Negative float converted to `uint8_t` | Scene/ship rendering |
  | `src/noctis.cpp:2869` | Float outside `int32_t` range | GOESnet session ID seeding |
  | `src/tdpolygs.h:949` | Float outside `int32_t` range | Texture mapping |

  The run was stopped by a five-second timeout. It does not establish whether
  further issues appear during longer play. `src/brtl.cpp` also uses signed
  32-bit multiplication for its PRNG state, which needs a focused deterministic
  test and overflow review; the short startup did not report that path.

The temporary checkout used for this audit was
`/tmp/noctis-lr-m0-audit-ZOxHnP/upstream`. It is outside this repository and
should be treated as disposable. The commands above use fresh example paths.

### Port follow-up (2026-09-26)

The imported port reproduced the first three diagnostics above in a bounded
five-second Clang ASan/UBSan run on the host display. It now decodes the
unaligned little-endian glyph words bytewise, applies explicit modulo-width
conversion to legacy floating seeds and effect values, and uses matching
variadic format types. The four integer `fabs` compiler warnings were also
removed. A repeat of the same graphical smoke reached the active loop with no
sanitizer diagnostic before the timeout. The earlier `tdpolygs.h` conversion
did not recur in either current five-second run; it remains a historical,
path-dependent finding rather than a resolved claim.

## Build and dependency inventory

| Component | Observed state | Evidence |
| --- | --- | --- |
| Build system | One CMake executable with four C++ sources: `noctis.cpp`, `noctis-1.cpp`, `noctis-0.cpp`, `brtl.cpp` | [`CMakeLists.txt`](https://github.com/dgcole/noctis-iv-lr/blob/e1b0817da580e23062b3d2a64b7b6947bc0cb419/CMakeLists.txt) |
| Dependency manager | Checked-in CPM bootstrap downloads CPM.cmake v0.43.1 with a SHA-256 check | [`cmake/CPM.cmake`](https://github.com/dgcole/noctis-iv-lr/blob/e1b0817da580e23062b3d2a64b7b6947bc0cb419/cmake/CPM.cmake) |
| Raylib | CPM tag 6.0; audit resolved commit `dbc56a87da87d973a9c5baa4e7438a9d20121d28` | Root CMake and downloaded Git checkout |
| GLM | CPM tag 1.0.3; audit resolved commit `8d1fd52e5ab5590e2c81768ace50c72bae28f2ed` | Root CMake and downloaded Git checkout |
| Language version | No `CMAKE_CXX_STANDARD` or target `cxx_std_*` declaration; compiler default currently governs | Root CMake |
| Compiler selection | Unix CMake overwrites requested compiler paths with `/usr/bin/clang` and `/usr/bin/clang++`; GCC 16 succeeds only after removing that override in a temporary test copy | Root CMake and generated build commands |
| Linux CI | One Ubuntu build and tar artifact job on pushes to `master`; no test invocation | [`.github/workflows/main.yml`](https://github.com/dgcole/noctis-iv-lr/blob/e1b0817da580e23062b3d2a64b7b6947bc0cb419/.github/workflows/main.yml) |
| Windows | README says it builds under MSYS2 but is especially buggy; no Windows CI at this commit | [`README.md`](https://github.com/dgcole/noctis-iv-lr/blob/e1b0817da580e23062b3d2a64b7b6947bc0cb419/README.md) and workflow |
| Web | A checked-in semi-functional Emscripten demo exists; the current CMake web branch uses developer-specific include and library paths | `web/` and root CMake |
| Platform headers | Active source includes POSIX headers such as `unistd.h`; Windows support needs direct verification | `src/noctis-d.h` |

The Linux configure detected X11 and built Raylib's bundled GLFW. The project's
Clang override and unstated C++ standard need resolution in M0-W08. CPM package tags are named,
but the two resolved commits above should be recorded when reproducible
dependency acquisition is implemented in M1-W03.

## Subsystem inventory

The active program is a mostly direct C++ translation of the original large
modules. Its main executable sources total about 17,000 lines; the active
`tdpolygs.h` renderer adds about 1,400. Many globals and DOS-era data layouts
remain. `src/Old/` contains legacy files and is not compiled.

| Subsystem | Current implementation | Confidence and evidence |
| --- | --- | --- |
| Platform and input | Raylib window, keyboard/mouse input, X11/OpenGL presentation | Builds and starts here; `src/noctis.cpp` and `src/noctis-0.cpp` |
| Indexed renderer | 320×200 index buffer, 256-color palette conversion, software polygon and texture routines, uploaded to a Raylib texture | Present and starts; visual parity untested; `src/tdpolygs.h`, `swapBuffers()` |
| Space game loop | Ship state, navigation, target selection, screens, travel, drawing | Present; upstream marks space logic and rendering complete; `src/noctis.cpp` |
| Universe generation | Borland-compatible PRNG functions, star and planet generation, texture generation | Present; upstream says matching has not been exhaustive; `src/brtl.cpp`, `src/noctis-0.cpp` |
| Planet surface | Terrain, sky, flora/fauna, ruins, weather, and `planetary_main()` code exist | Incomplete by upstream's own checklist; `src/noctis-1.cpp` |
| Save/load | Explicit sequence of raw field writes and reads for `current.bin`; surface and starmap file operations | Present, but not tested or schema-validated; `freeze()` and `unfreeze()` |
| Resources | `supports.nct` loaded from `res/` using legacy offsets; NCC, VOC, maps, and text resources tracked | Present; resource loading was reached on startup; `sa_open()` |
| GOESnet | Command line still invokes `system()` and exchanges files; three translated module sources exist | Incomplete; `src/goesnet/` is absent from CMake target |
| Legacy code | `src/Old/` holds former assembly-era utilities and modules | Reference material only; absent from CMake target |

Upstream's own [port-status checklist](https://github.com/dgcole/noctis-iv-lr/blob/e1b0817da580e23062b3d2a64b7b6947bc0cb419/README.md)
marks planet surface generation, planetary object rendering, and GOESnet modules
unfinished. It marks space logic, generation, rendering, planetary logic, and
saving/loading complete. Those are project claims, not verified parity results.

## Known gaps and migration implications

| Gap at this commit | Direct evidence | Roadmap implication |
| --- | --- | --- |
| No tracked `data/` or `gallery/` directories | Git tree has neither; code opens `data/current.bin`, `data/STARMAP.BIN`, and `gallery/SNAP*.BMP` | M1 packaging and M5 persistence must establish writable paths and initial data |
| Runtime save can fail silently when `data/` is absent | `freeze()` returns if `fopen(..., "wb")` fails | M5 needs error reporting and save tests |
| GOESnet remains shell-driven | `run_goesnet_module()` calls `system()`; CMake builds none of `src/goesnet/*.cpp` | M5-W04/W05 remain substantial work |
| Planet surface and objects are partial | README unchecked boxes; active code exists but has no fixtures | M4 characterization must precede parity claims |
| Procedural parity is unverified | README explicitly says testing has not been exhaustive; no test target | M2 fixture work is essential |
| Renderer has known rough spots | `src/tdpolygs.h` contains TODOs including an untested path and `facing()` fix | M3 visual fixtures and sanitizer work |
| Sanitizer reports undefined behavior during startup | Four UBSan diagnostics in `noctis.cpp` and `tdpolygs.h`; see check above | M1/M3 need a repeatable sanitizer smoke path and fixes before compatibility claims |
| Platform coverage is narrow | Only Linux build CI; README reports Windows bugs | M7 Windows build and runtime validation |
| CMake embeds environment assumptions | Hard-coded Unix Clang paths, `/home/dcole/...` web paths, symlinked resources; GCC selection is overwritten | M1/M0-W08 build normalization |
| Program architecture is tightly coupled | Broad globals across the three Noctis modules; platform and simulation code share files | Introduce seams incrementally during vertical slices |
| Permission and asset provenance need review | Upstream README references WTOF terms and an author condition; this audit did not verify the permission chain | M0-W04 is required before redistribution decisions |

The local Noctis IV Plus repository has `data/STARMAP.BIN` and
`data/GUIDE.BIN`, but copying them into LR would be a separate integration and
provenance decision. The M0-W06 feature ledger will identify which Noctis IV
Plus behavior is absent from this LR baseline.

The LR `res/supports.nct`, `res/globes.map`, and `res/offsets.map` files are
byte-identical to the corresponding NIV+ files in this repository by SHA-256.
That establishes reuse of these specific resource bytes; it does not establish
identical generated worlds or settle asset permissions. LR's `.gitignore`
explicitly excludes `data/` and `gallery/`, explaining their absence from the
tracked tree but not how fresh installations initialize those paths.

## Recommended handoff to M0-W03 and M0-W06

1. Treat the audited commit as a reproducible **candidate baseline**. Record
   final adoption in an ADR after choosing repository/history integration.
2. Preserve the LR Git history and exact dependency versions during import.
3. Establish first-party `data/` path handling early. A successful window
   launch is not evidence that save/load works.
4. Build the NIV+ ledger by comparing `source/docs/NIVPLUS_CHANGES.TXT`, local
   commits, and active LR behavior. Planet surfaces, GOESnet, and saves deserve
   early fixture coverage.
5. Recheck this audit if a newer LR commit is proposed as the baseline.
