# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-01.
- Repository: local `Noctis-IV-OM`, remote project `aedmark/Noctis-IV-OM`.
- Branch: `master`; Milestone M8 (Stabilization and 1.0 GA) in progress.
- Status: M8-W01 through M8-W05 are **DONE**; M8-W06 is **READY** for 1.0 tagging and publication.
- Public-facing progress recorded in `devlog.html`.

## Read first

1. `ROADMAP.md` for milestone statuses and exit criteria.
2. `BUILDING.md` for multi-compiler presets, package builds, and offline cache.
3. `CONTRIBUTING.md` for architectural rules, coding standards, and quality gates.
4. `TROUBLESHOOTING.md` for diagnostics, graphics fallbacks, and recovery workflows.
5. `docs/porting/PERFORMANCE_BUDGETS.md` for timing and memory performance baselines.
6. `docs/porting/RELEASE_RUNBOOK.md` for the step-by-step 1.0 release and rollback procedure.

## Completed this session

- **M8-W01 (Defect Resolution & Memory Safety):**
  - Resolved the critical lithium depletion on game relaunch.
  - Hardened 2D flare line drawer (`stick`) with 2D viewport clipping checks, preventing 1–3 byte buffer overflows past the 64,000-byte video buffer.
  - Fixed operator precedence in additive polygon flare blending (`src/tdpolygs.h`) and added an automated regression test in `tests/renderer_fixture_test.cpp`.
  - Fixed font glyph underflow in `digit_at` (`src/noctis.cpp`), restoring row-0 rendering for `$`, `[`, `]`, and `^`.
  - Guarded `NOMINMAX` macro against redefinition in MinGW.
  - Replaced non-portable `gcvt` with standard `std::snprintf("%.15g")` in `alphavalue`.
  - Added defensive screen buffer bounds check in `single_pixel_at_ptr`.
  - Verified 100% test pass rate (40/40) across all build presets: `linux-clang-release`, `linux-clang-debug`, `linux-gcc-debug`, and `linux-clang-sanitized` (ASan/UBSan with zero leaks and zero errors).

- **M8-W02 (Documentation & Community Polish):**
  - Created [`CONTRIBUTING.md`](../../CONTRIBUTING.md) with comprehensive contribution guidelines, code style standards, and verification steps.
  - Created [`TROUBLESHOOTING.md`](../../TROUBLESHOOTING.md) covering preflight diagnostics, Mesa/OpenGL software rendering fallbacks (`LIBGL_ALWAYS_SOFTWARE=1`), XWayland, the GOESnet terminal focus trap, the `--standard-drive` power loss recovery workflow, data paths, and audio status.
  - Updated [`PACKAGE_README.md`](../../PACKAGE_README.md), [`PACKAGE_README_WINDOWS.md`](../../PACKAGE_README_WINDOWS.md), [`README.md`](../../README.md), and [`BUILDING.md`](../../BUILDING.md).
  - Bundled `TROUBLESHOOTING.md` into release packages and updated package verification scripts (`VerifyLinuxPackage.cmake`, `VerifyWindowsPackage.cmake`).

- **M8-W03 (Performance & Memory Budgets):**
  - Established formal budgets in [`docs/porting/PERFORMANCE_BUDGETS.md`](PERFORMANCE_BUDGETS.md).
  - Empirical profiling of the 817-frame orbit-to-surface journey measured 1.98 ms per frame software rasterization (~504 FPS unthrottled), preserving over 96.4% CPU headroom under the 55.0 ms DOS simulation tick deadline.
  - Measured headless peak RSS memory at 7.2 MB (under the 32 MB budget) and full OpenGL 3.3 windowed presentation at 91.9 MB (under the 150 MB budget).
  - Confirmed zero memory leaks under AddressSanitizer.

- **M8-W04 (Licensing & Distribution Integrity):**
  - Audited and verified all license notices and attributions: `WTOF-LICENSE.md`, `LICENSE`, `CONTRIBUTORS.md`, `THIRD_PARTY_NOTICES.md`, and `licenses/` directory (Raylib zlib license, GLM MIT/Happy Bunny license).
  - Verified starmap and guide catalog hashes against `CONTENT_MANIFEST.json`.
  - Enforced exact file manifests in both Linux `.tar.gz` and Windows `.zip` packages.

- **M8-W05 (Release & Rollback Rehearsal):**
  - Authored [`docs/porting/RELEASE_RUNBOOK.md`](RELEASE_RUNBOOK.md) documenting release quality gates, artifact generation, checksum verification, isolated extraction testing, publication steps, and rollback procedures.
  - Executed a successful packaging rehearsal: generated Linux and Windows packages, verified checksums, extracted archives into a clean directory, and confirmed both headless diagnostics and graphical smoke tests passed with zero errors.

- **Telemetry & Tracking:**
  - Updated [`ROADMAP.md`](ROADMAP.md) marking M8-W01 through M8-W05 as `DONE`.
  - Updated public dev diary [`devlog.html`](../../devlog.html) with the Milestone 8 stabilization and 1.0 readiness update.

## Test suite and package status

- **Clang Release:** 40/40 passed (3.09s).
- **Clang Debug:** 40/40 passed (3.10s).
- **GCC Debug:** 40/40 passed (14.52s).
- **Clang Sanitized (ASan + UBSan):** 40/40 passed (50.81s, zero leaks, zero UB).
- **MinGW Windows Cross-Compilation:** Built cleanly with zero warnings.
- **Linux Package (`.tar.gz`):** Built, checksummed, verified with `VerifyLinuxPackage.cmake`.
- **Windows Package (`.zip`):** Built, checksummed, verified with `VerifyWindowsPackage.cmake`.
- **Extracted Smoke Test:** Passed `--diagnostics` and `--graphical-smoke` with exit code 0.

## Next steps (M8-W06)

1. Review and prepare final 1.0 version strings (e.g. `1.0.0` in `CMakeLists.txt`).
2. Create annotated git tag `v1.0.0`.
3. Build final release archives (`.tar.gz` and `.zip`) and checksums (`.sha256`).
4. Push tag and commit to `origin/master`.
5. Publish the GitHub Release for 1.0 General Availability.
