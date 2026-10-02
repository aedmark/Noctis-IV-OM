# M7-W02 Linux packaging

Status: complete. GitHub Actions run
[`36795944060`](https://github.com/aedmark/Noctis-IV-OM/actions/runs/36795944060)
passed the complete package workflow on a fresh Ubuntu 24.04 x86-64 runner.

## Support boundary

The preview artifact is a portable directory archive for x86-64 Ubuntu 24.04.
That is the distribution and architecture verified by the clean packaging
runner. The game uses Raylib's X11 backend and can run in a Wayland session
through XWayland; the artifact does not claim a native Wayland backend or
untested Linux distributions.

Players need ordinary X11, OpenGL, and audio runtime libraries. They do not
need the compiler, CMake, Ninja, or any `-dev` packages. The exact Ubuntu 24.04
runtime command is included in the package README.

## Artifact contract

`linux-clang-release` creates a release-mode build. CPack installs only the
project's `Runtime` component into
`Noctis-IV-OM-linux-x86_64-preview.tar.gz` and writes a SHA-256 checksum beside
it. The archive has one top-level directory containing:

- `nivlr`, the native executable;
- the complete `res/` runtime resource directory;
- verified October 2023 `defaults/STARMAP.BIN` and `defaults/GUIDE.BIN` seeds;
- player instructions, project and inherited license terms, contributor
  credits, and complete Raylib/GLM license texts.

The packaging component deliberately excludes headers, libraries, source,
tests, historical DOS executables, the Borland archive, the manual/soundtrack
material outside the cleared package scope, and build dependencies.

## Verification

`cmake/VerifyLinuxPackage.cmake` extracts the artifact into a new directory,
requires the documented top-level layout and key files, rejects common source
or legacy-binary leaks, runs the packaged executable's headless diagnostics
from the extracted root. M7-W04 revised this check to create an isolated
OS-style player profile headlessly, verify both seeded catalogs plus its
`data/`, `gallery/`, `movies/`, and `config/` directories, and confirm that no
mutable directories are required inside the package.

`.github/workflows/linux-package.yml` repeats the whole operation on a fresh
Ubuntu 24.04 runner:

1. install the declared X11 build dependencies and Xvfb;
2. configure and build the release preset;
3. pass all 38 tests in release mode;
4. generate the archive and verify its checksum;
5. extract and validate the package rather than the build tree;
6. open the packaged game under Xvfb with Mesa software rendering, present the
   bounded graphical-smoke frames, and exit cleanly;
7. upload the verified archive and checksum as one CI artifact.

The local implementation pass completed steps 2–5 on 2026-09-30. Clean hosted
run `36795944060` then completed all seven steps, including the extracted
artifact's software-rendered graphical smoke, and uploaded the archive plus
checksum. That run is the M7-W02 clean-distribution acceptance evidence for the
original colocated layout. M7-W04's revised layout passes the local verifier;
its next clean hosted run will become the current-layout evidence.
