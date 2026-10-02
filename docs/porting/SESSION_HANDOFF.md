# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-01.
- Repository: local `Noctis-IV-OM`, remote project `aedmark/Noctis-IV-OM`.
- Branch: `master`; Milestone M8 (Stabilization and 1.0 GA) in progress.
- Status: Milestone M8 (Stabilization and 1.0 GA) is **DONE**; 1.0 General Availability published.
- Public-facing progress recorded in `devlog.html`.

## Read first

1. `ROADMAP.md` for milestone statuses and exit criteria.
2. `BUILDING.md` for multi-compiler presets, package builds, and offline cache.
3. `CONTRIBUTING.md` for architectural rules, coding standards, and quality gates.
4. `TROUBLESHOOTING.md` for diagnostics, graphics fallbacks, and recovery workflows.
5. `docs/porting/PERFORMANCE_BUDGETS.md` for timing and memory performance baselines.
6. `docs/porting/RELEASE_RUNBOOK.md` for the step-by-step release and rollback procedure.

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

- **M8-W06 (Tag and Publish 1.0 General Availability):**
  - Updated version to `1.0.0` in `CMakeLists.txt`, `CHANGELOG.md`, `README.md`, `PACKAGE_README.md`, and `PACKAGE_README_WINDOWS.md`.
  - Built production release archives: `Noctis-IV-OM-linux-x86_64.tar.gz` and `Noctis-IV-OM-windows-x86_64.zip`.
  - Generated and verified SHA-256 checksums.
  - Staged release assets in `public-builds/1.0.0/`.
  - Updated homepage [`index.html`](../../index.html) download links, specs, and release banner.
  - Updated public dev diary [`devlog.html`](../../devlog.html).
  - Tagged `v1.0.0` and pushed to `origin/master`.

- **Release 1.0.1 (Patch Release):**
  - Resolved landing descent soft-lock on low-gravity worlds like Oakenshield P01 with a descent acceleration floor.
  - Enabled `Escape` key abort during descent to safely return to cockpit.
  - Added atmospheric starlight floor and terrain ambient illumination floor for dim-star systems (`dfs <= 0.2`).
  - Protected `ip_targetted` against power-loss reset while on planetary surfaces (`surface_active` / `SurfaceActiveScope`).
  - Added defensive nearest-body recovery for `ip_targetted` on surface session resume.
  - Added `oakenshield_landing_fixture` automated test (41/41 passing test suite).
  - Restored `Page Up` (raise helmet visor / open suit HUD) and `Page Down` (lower helmet visor / close suit HUD) on planetary surfaces.
  - Restored `Page Up` / `Page Down` full-page scrolling in the GOES Guide reader.
  - Mapped `Home` and `End` keys for GOES prompt clearing and boundary scrolling.
  - Packaged, checksummed, verified, and tagged `v1.0.1`.

- **Release 1.1.0 (Feature Release — The Torchlight Update):**
  - Added planetary Suit Torch / Headlamp toggled with `L` key with authentic spotlight cone, ambient lighting, and quadratic falloff in 32-bit RGBA color space.
  - Sharpened HUD status messages (`TORCH ON`, `TORCH OFF`, `MOUSELOOK ENABLED`, etc.) rendered at native resolution directly over the retro framebuffer.
  - Automatically preserve spaceship situation state (`current.niv`) via `freeze()` when quitting from planetary surfaces via `Escape`.
  - Reconstruct star system on cold restart surface resume (`_delay = 0`).
  - Guarded orbital mechanics (`planet_xyz`, `moonorigin`, `rtp`) against division by zero and `NaN` on empty systems.
  - Hardened HUD telemetry text renderer (`wrouthud`) against buffer overflows and sanitized telemetry values.
  - Safely discard orphaned surface checkpoints without process abort.
  - Verified 41/41 passing test suite across all compiler configurations.
  - Packaged, checksummed, verified, and tagged `v1.1.0`.

## Test suite and package status

- **Clang Release:** 41/41 passed (4.01s).
- **GCC Debug:** 41/41 passed (16.78s).
- **Clang Sanitized (ASan + UBSan):** 41/41 passed (57.86s, zero leaks, zero UB).
- **MinGW Windows Cross-Compilation:** Built cleanly with zero warnings.
- **Linux Package (`.tar.gz`):** Built, checksummed, verified with `VerifyLinuxPackage.cmake`.
- **Windows Package (`.zip`):** Built, checksummed, verified with `VerifyWindowsPackage.cmake`.
- **Extracted Smoke Test:** Passed `--diagnostics` and `--graphical-smoke` with exit code 0.

## Next steps

1. Monitor community feedback on 1.1.0 Torchlight release.
2. Plan subsequent post-1.1 feature updates (e.g. procedural audio engine integration, Wayland native backend, high-DPI scaling).
