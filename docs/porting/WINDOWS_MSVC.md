# M7-W01 Windows MSVC validation

Status: complete. Hosted compilation and the platform-independent suite pass;
the portable MSVC Release build also runs graphically on native Windows and
passes its bounded three-frame startup under Wine.

## CI boundary

`.github/workflows/windows.yml` uses the pinned GitHub-hosted `windows-2022`
image and the `windows-msvc-debug` CMake preset. Visual Studio 2022 builds the
application and all test targets as C++20 with `/fp:strict` on
compatibility-sensitive targets and `/Zc:__cplusplus` for accurate diagnostics.

The required lane runs every test except those labeled
`cross_platform_review` or `requires_graphics`. This is 34 platform-independent
tests covering diagnostics, both PRNGs, numeric semantics, saves and legacy
migration, persistence across processes, controls, Moviemaker, panorama
publication, GOESnet data and workflows, indexed presentation, the software
polygon fixture, travel, scripted flight, landing state, generators, and all
four DOS-confirmed stars.

The first required green run is
[GitHub Actions run 36793746326](https://github.com/aedmark/Noctis-IV-OM/actions/runs/36793746326)
at commit `24ed729`. It configured, built, passed all 34 required tests, and
uploaded the provisional runtime artifact on the hosted Windows 2022 image.

## Defects found and fixed

The first hosted build exposed five inconsistent declarations hidden by the
Linux object format: two text buffers had different `char`/`int8_t` types
across translation units, and three read-only description tables had mutable
signed-byte declarations. Their declarations now match their definitions.

The first complete test run exposed a filesystem semantic difference. C
`rename()` replaces an existing destination on Linux but not on Windows. A
shared same-filesystem replacement boundary now uses `MoveFileExW` with
replace-existing and write-through flags on Windows and `rename()` on POSIX.
Native saves, panorama publication, starmap updates, and guide updates all use
that boundary. The affected tests pass on MSVC and remain green on Linux.

## M7-W05 disposition

- `representative_surface_fixtures`, `surface_environment_fixtures`,
  `surface_content_fixtures`, and `orbit_surface_journey` retain matching
  semantic/structural results with selected compiler-specific buffer and frame
  hashes. M7-W05 reviewed and published the boundary, added exact MSVC Release
  baselines, and made all four tests gating for the shipped Windows build. See
  `M7_COMPATIBILITY_REPORT.md` and run `36869013306`.
- The hosted runner still cannot run `windows_graphical_startup` because its VM
  has no OpenGL driver. This is an infrastructure limitation, not the runtime
  evidence used to close the work item.
- The uploaded debug runtime is CI evidence only. M7-W03 now provides the
  separately verified portable Release ZIP; see `WINDOWS_PACKAGING.md` and run
  `36796703199`.

MSVC Debug retains a recorded non-blocking presentation probe because its
selected hashes differ from the shipped Release configuration.

## Real-machine command

Download the `Noctis-IV-OM-windows-x86_64-preview` artifact from package run
[`36796703199`](https://github.com/aedmark/Noctis-IV-OM/actions/runs/36796703199),
extract the ZIP on a normal 64-bit Windows 10 or 11 PC, open PowerShell in the
extracted directory, and run:

```powershell
.\nivlr.exe --graphical-smoke
```

It presents three frames and exits without keyboard or mouse input.

On 2026-09-30, the project maintainer confirmed that the extracted portable
build runs on native Windows and under Wine. The same extracted `nivlr.exe`
(SHA-256
`2551502975bb612bde6ecaca397f5e1f5c44bf8066ba6733aaa725828ad9a1ed`)
was then run with `--diagnostics` and `--graphical-smoke` under Wine. It reported
MSVC 19.44, loaded the required resources, opened the Win32 Raylib backend on an
AMD Radeon RX 7800 XT through OpenGL 4.6, presented three frames, emitted the
`graphical_smoke` success event, and shut down cleanly with status 0. Together,
the native-Windows confirmation and reproducible Wine transcript close M7-W01
and the P18 native-startup outcome.
