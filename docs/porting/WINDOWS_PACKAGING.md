# M7-W03 Windows packaging

Status: complete. GitHub Actions run
[`36796703199`](https://github.com/aedmark/Noctis-IV-OM/actions/runs/36796703199)
passed the complete package workflow on a fresh hosted Windows 2022 x86-64
runner and uploaded the ZIP plus checksum.

## Artifact boundary

`windows-msvc-release` produces a 64-bit MSVC Release build with the Microsoft
C/C++ runtime linked into the executable. CPack installs only the project
`Runtime` component into `Noctis-IV-OM-windows-x86_64-preview.zip` and writes a
SHA-256 checksum beside it.

The extracted package mirrors the verified Linux artifact where the operating
systems permit: `nivlr.exe`, the complete `res/` directory, verified October
2023 starmap/guide seeds under immutable `defaults/`, Windows-specific player
instructions, credits, project and
inherited terms, and complete Raylib/GLM license texts.

It deliberately excludes source, tests, headers, import libraries, object and
debug-symbol files, legacy DOS executables, the Borland archive, and media
outside the cleared package scope. It is a portable user-owned directory, not
an installer, and requires no administrative installation step.

## Hosted verification

`.github/workflows/windows-package.yml` runs on a fresh hosted Windows 2022
x86-64 environment and must:

1. configure and build the MSVC Release preset;
2. pass all 40 non-graphical tests, including exact MSVC Release presentation
   baselines; the graphical check remains assigned to a real PC;
3. create the ZIP and independently verify its SHA-256 checksum;
4. extract the artifact into a new directory and enforce its exact runtime
   boundary;
5. run `nivlr.exe --diagnostics` from the extracted root, then use
   `--prepare-user-data` with an isolated profile and verify both seeded
   catalogs plus the `data`, `gallery`, `movies`, and `config` directories;
6. inspect the packaged executable's dependencies and reject a remaining
   Visual C++ Redistributable DLL requirement;
7. upload the ZIP and checksum as a single CI artifact.

`cmake/VerifyWindowsPackage.cmake` owns the extracted-package checks. Run
`36796703199` passed all seven steps, including 34/34 required tests and the
no-redistributable dependency inspection. The uploaded 2,217,922-byte ZIP was
then downloaded on Linux, its published checksum independently verified, and
all 36 archive entries inspected. This closes M7-W03's reproducible package
production boundary for the original colocated layout. M7-W04 changes the
package contract so mutable files use Local/Roaming AppData. M7-W05 run
`36869013306` supersedes that artifact: it passes all 40 non-graphical tests,
ships `KNOWN_ISSUES.md`, verifies the revised profile contract, and uploads the
same-commit W06 candidate ZIP.

## Real graphics boundary

GitHub's hosted Windows VM has no usable OpenGL driver. The ZIP README therefore
provides `nivlr.exe --graphical-smoke`, which opens the real window, presents
three frames, and exits without input. Running that command on an ordinary
64-bit Windows 10 or 11 PC is the remaining M7-W01/P18 graphical evidence; it
is not disguised as a hosted packaging check.

This preview is unsigned, so Windows may display a SmartScreen warning. Native
Windows and Wine startup are confirmed under M7-W01. M7-W05 publishes and
enforces the accepted platform-specific presentation hashes.
