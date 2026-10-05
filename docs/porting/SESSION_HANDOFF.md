# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-04.
- Repository: local `Noctis-IV-OM`, remote project `aedmark/Noctis-IV-OM`.
- Branch: `master`; Milestone M12 (Celestial Cartography & Waypoint Navigation) complete.
- Status: Release 2.0.1 (The Storage Reset & Fresh Start Patch) packaged, tagged, and published.
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
- **Release 1.3.0 (Feature Release — The Aural Update):**
  - Designed and implemented native, self-contained real-time procedural audio synthesis engine (44.1 kHz, 32-bit float, miniaudio backend) with zero external sound files or downloaded assets.
  - Synthesized meditative, non-fatiguing Stardrifter cabin drone: 55 Hz fundamental, 27.5 Hz sub-bass, 110 Hz harmonic, quiet CRT monitor purr, and dual-filtered pink noise life-support ventilation breathing.
  - Synthesized deep space observation deck cosmic silence and diffuse stereo solar wind sweeps (`ontheroof`).
  - Implemented dynamic Vimana warp propulsion acoustics: physical speed-responsive frequency starting low at ignition (~28 Hz), accelerating into a rhythmic gravitic oscillation at cruising warp (~96 Hz), smooth deceleration during arrival, and 1.8-second arrival spool-down without abrupt cuts.
  - Modeled planetary atmospheric acoustics with 2-pole resonant SVF tracking surface pressure (`pp_pressure`) and atmospheric existence (`atmosphere`). Airless worlds feature vacuum silence with interior suit life-support hum; atmospheric worlds produce howling wind gusts, rain droplet hiss, and distant thunder.
  - Added tactile procedural exploration Foley: suit torch click on `L`, visor servo glide on `Page Up`/`Page Down`, cold-gas thruster burst on `Space`, and regolith footsteps.
  - Added global mute hotkey (`F9` or `Ctrl+M`), `--no-audio` CLI flag, and safe headless fallback.
  - Fixed observation deck runaway CPU time-warp bug: eliminated legacy DOS `ROOFSPEED` frame-limiter bypass, guaranteeing canonical 18.2 FPS (55 ms per tick) simulation pacing on deck.
  - Fixed WASD backward movement key collision: removed rogue legacy `'s'` key intercept to restore clean backward walking.
  - Added `audio_determinism` unit test, maintaining 100% test pass rate (42/42 tests).
  - Packaged, checksummed, verified, and tagged `v1.3.0`.

- **M11-W02 (Flight & Maneuvering Acoustics):**
  - Synthesized sublight RCS attitude thruster acoustics: crisp cold-gas valve pop on maneuver onset (`play_rcs_burst()`, 0.12s, 2100 Hz bandpass) plus continuous subtle stereo thruster hiss (`filter_rcs_hiss_l/r` at 1900 Hz, gain ~0.22 smoothed) engaged during spacecraft navigation pitch/yaw steering (`dlt_nav_beta`) and collision avoidance corrections.
  - Synthesized atmospheric entry buffeting turbulence: physical descent velocity (`gravity`) and air pressure (`pp_pressure`) dynamically drive pink noise through 2-pole resonant lowpass filter (65–125 Hz, $Q=1.8$) modulated by a 6.5 Hz turbulent LFO during planetary descent.
  - Synthesized dual-stage physical touchdown clunk (`play_touchdown_clunk()`): 0.40s duration featuring low-frequency hull thud with pitch dropping from 85 Hz to 35 Hz, dual damped metallic latch rings (720 Hz and 1150 Hz), and surface regolith compression crunch on landing impact and ground bounce.
  - Synthesized tactile Cockpit & GOESnet Foley:
    - `play_cockpit_button()`: Tactile dashboard rocker switch / console button click on console operations, FCS commands, and bulkhead screen selection.
    - `play_terminal_keystroke()`: Mechanical vintage solenoid/spring keyboard typing clacks (3 round-robin procedural variations) for GOESnet typing, star catalog prompts, and celestial labeling.
    - `play_goesnet_transmit()`: Stepped frequency telemetry chirp (1050 -> 1680 -> 2520 Hz) on command submission (`Enter`).
    - `play_goesnet_chime(bool)`: Dual-harmonic bell acknowledge chime (880 + 1320 Hz) on success/locks vs retro dual square/sine buzz (185 + 245 Hz) on error/rejection/out-of-range.
    - `play_terminal_scroll()`: Subtle linefeed scroll tap on Guide browsing and landing target coordinate cursor adjustments.
    - `play_deck_lift()`: Hydraulic motor servo whine (180 -> 240 Hz with 40 Hz PWM) on observation deck elevator movement.
  - Verified 100% test pass rate across all 45 automated test suites.
- **Release 1.8.0 (Feature Release — The Acoustics & Surface Exploration Update):**
  - Synthesized flight & maneuvering acoustics: sublight RCS thrusters (onset cold-gas valve pop + continuous stereo hiss), dynamic atmospheric entry buffeting turbulence, dual-stage physical touchdown clunk.
  - Decoupled observation deck camera look/step from spacecraft attitude thrusters.
  - Synthesized tactile cockpit & GOESnet foley: mechanical keyboard clacks (3 procedural variations), telemetry transmit chirps, dual-harmonic acknowledge / buzz cues, console rocker buttons, linefeed scroll taps, deck elevator servo.
  - Surface exploration polish: true albedo luminance scaling for suit torch (`L`), strictly synchronized footstep pacing, soothing spacesuit life-support ventilation hum on airless worlds, and 1,200-unit ceiling jetpack flight dynamics without uphill collision hitches.
  - Rebuilt and packaged Linux (`.tar.gz`), Windows (`.zip`), and Web (`.zip`) releases with SHA-256 checksums in `public-builds/1.8.0/`.
  - Updated all documentation, manifests, and websites.
- **Release 1.9.0 (Feature Release — The Audio Controls & Exploration Ergonomics Update):**
  - Implemented 5 independent audio volume categories: Master, Spaceflight & Maneuvering, Cockpit Foley, Visor & Suit, Environment & Surface.
  - Designed interactive High-DPI Audio Options overlay in graphics menu (`F2` or `Tab`/`A`), with mouse slider dragging, keyboard stepping (`+`/`-`), number keys (`1`–`5`), mute toggle (`M`), and automatic persistence in `config.ini`.
  - Resolved cursor unlocking and screenshot ergonomics: pressing `F10` immediately freezes camera and player locomotion, clearing residual rotational momentum (`dlt_alfa`, `dlt_beta`) and translation (`shift`, `step`) so external screenshot apps capture stable views.
  - Gated inputs while unfocused or unlocked, and eliminated cursor recentering warp jump on focus restoration or `F10` re-lock.
  - Verified 100% test pass rate across all 45 automated test suites.
  - Rebuilt and packaged Linux (`.tar.gz`), Windows (`.zip`), and Web (`.zip`) releases with SHA-256 checksums in `public-builds/1.9.0/`.
- **M11-W01 (In-Engine Media Export):**
  - Added direct in-engine media export in the <kbd>F4</kbd> Image Archive Viewer: pressing <kbd>D</kbd> or clicking the Export button immediately triggers export.
  - Web: triggers browser download via Emscripten JavaScript bridge to user's Downloads folder.
  - Desktop: exports snapshot or panorama to system Downloads folder or reveals gallery in file explorer.
- **M11-W03 (Native Gamepad & Joystick Flight Controls):**
  - Integrated dual-stick analog flight and surface locomotion via Raylib Gamepad API.
  - Analog camera yaw and pitch with customizable deadzone filtering (`0.05`–`0.50`, default `0.15`) and sensitivity scaling.
  - Trigger thrusters: left trigger (<kbd>LT</kbd>) decelerates / reverses, right trigger (<kbd>RT</kbd>) accelerates.
  - Attitude maneuvering: left bumper (<kbd>LB</kbd>) and right bumper (<kbd>RB</kbd>) roll the spacecraft in flight, or activate vertical jetpack thrust on planetary surfaces.
  - Haptic dual-motor rumble feedback responding to propulsion burn onset, cold-gas RCS thruster bursts, atmospheric descent buffeting turbulence, touchdown impacts, and collision contacts. Toggleable with <kbd>R</kbd> or via the controls menu overlay.
- **M11-W04 (Configurable Controls & Sensitivity Persistence):**
  - Corrected mouse pitch inversion default so push-forward looks UP by default. Toggled with <kbd>I</kbd> or in Controls menu.
  - Mouse sensitivity scaling (0.1x to 5.0x, default 1.0x).
  - Interactive High-DPI Controls Options overlay in the graphics menu (<kbd>F2</kbd> or <kbd>Tab</kbd>/<kbd>C</kbd>) showing connected gamepad name, rumble status, sensitivity, deadzones, and input bindings.
  - Settings persist across launches in `controls.ini` in user config directory.
- **Bug Fixes:**
  - Isolated arrow keys from `WASD` character movement during planetary landing zone coordinate selection (`active_screen == 2`), preventing coordinate adjustments from moving the character or triggering the observation deck elevator.
  - Re-mapped gamepad <kbd>B</kbd> button to context-sensitive cancel / right-click deselect instead of raw <kbd>Escape</kbd>, preventing accidental immediate exits to desktop.
- **M12-W01 (In-Engine Captain's Flight Log):**
  - Implemented automated flight log engine (`src/flight_log.h`, `src/flight_log.cpp`):
    - Real-time logging of interstellar jump arrivals with calculated light-year jump distances (`SystemArrival`).
    - Local orbital insertion logging with body ID and type (`OrbitArrival`).
    - Surface landing touchdown logging with exact planetary latitude/longitude coordinates (`SurfaceLanding`).
    - Object discovery/naming logging with custom labels and body identifiers (`LabelAssigned`).
    - Deterministic JSON loading/saving (`flight_log.json`) and Markdown export (`flight_log.md`).
  - Integrated flight log into engine lifecycle (`src/noctis.cpp`, `src/noctis-1.cpp`):
    - `restore_situation()` initializes log and logs starting system.
    - Remote jump arrival (`stspeed == 1`) logs interstellar transitions.
    - Local travel arrival (`ip_reaching`) logs orbit insertions.
    - Surface landing (`planetary_main`) logs touchdown events with planetary coordinates.
    - Cockpit console labeling (cases 1 and 2) logs star and planet designations.
  - Implemented GOESnet commands (`src/goesnet_commands.cpp`, `src/goesnet_protocol.cpp`):
    - `LOG` / `JOURNAL`: 21-column pager showing flight statistics (total jumps, landings, light-years traveled, objects named) and chronological event ledger (`LOG <N>` for page jumps).
    - `LOG EXPORT`: exports formatted flight journal to `flight_log.md` and JSON data to `flight_log.json` in user runtime directory.
    - `NAME <LABEL>` / `NAME STAR:<LABEL>` / `NAME P<N>:<LABEL>`: assigns names to unnamed stars and planetary bodies directly from GOESnet terminal, atomically appending to `STARMAP.BIN` via `append_starmap_label()`.
  - Added unit test suite `tests/flight_log_test.cpp` and expanded `tests/goesnet_commands_test.cpp`.
- **M12-W02 (Starmap Bookmarks & Waypoint Navigation System):**
  - Created `src/bookmarks.h` and `src/bookmarks.cpp`:
    - Full CRUD bookmark management (`add`, `get`, `remove`, `clear`, `find_by_star_id`).
    - INI serialization (`bookmarks.ini`) preserving star IDs, celestial coordinates, planetary indices, surface coordinates, and timestamps across sessions.
    - GOESnet pager formatter `format_goes_list` enforcing strict 21-column width limits, multi-page navigation (`BM 2`), active target marker `*`, and distance in light years or local body proximity.
  - Registered `GoesCommand::bookmarks` (`BM`, `BOOKMARK`, `WAYPOINT`) in `src/goesnet_protocol.cpp` and implemented `handle_bookmarks_command` in `src/goesnet_commands.cpp`:
    - Supports `BM LIST`, `BM ADD [label]`, `BM GOTO <id>`, `BM DEL <id>`, and `BM CLEAR`.
    - `BM GOTO` triggers remote jump lock (`set_remote_target`) or local body targeting (`set_local_target`).
  - Added cockpit shortcut: press <kbd>J</kbd> in cockpit to jump directly to Bulkhead Screen 1 (GOESnet) with `BM_` queued.
  - Added surface shortcut: press <kbd>J</kbd> while walking on a planetary surface to drop an instant GPS surface waypoint at current latitude/longitude.
  - Added unit test suite `tests/bookmarks_test.cpp`.
- **M12-W03 (Surface & Orbital Navigation HUD — The Explorer's Visor):**
  - Created `src/navigation_hud.h` and `src/navigation_hud.cpp`:
    - Dynamic 360° cardinal compass tape computation (`compute_heading`) with 8 cardinal headings (N, NE, E, SE, S, SW, W, NW) and digital heading degrees.
    - Planetary coordinates telemetry computation (`compute_surface_coordinates`) formatting latitude, longitude, altitude AGL, and elevation MSL in meters.
    - Lander Return Beacon guidance computation (`compute_lander_beacon`): calculates distance in meters/km and 8-way relative directional arrow (`[^]`, `[^>]`, `[>]`, `[v>]`, `[v]`, `[<v]`, `[<]`, `[<^]`) pointing back to the landed capsule or `DOCKED`.
  - Integrated into surface rendering in `src/noctis-0.cpp`:
    - Added Visor HUD modes: `standard`, `explorer_telemetry`, and `minimal`.
    - Protected headless regression test fixtures (`environment_fixture_mode`, etc.) so deterministic pixel hashing is preserved.
  - Added surface hotkey: press <kbd>V</kbd> to cycle Visor HUD modes with authentic suit servo audio feedback.
  - Added unit test suite `tests/navigation_hud_test.cpp`.
- **Release 2.0.0 (The Celestial Cartography & Explorer's Update):**
  - Packaged Linux, Windows, and Web release archives in `public-builds/2.0.0/` with SHA-256 checksums.
  - Tagged `v2.0.0` and pushed to remote master.
- **Release 2.0.1 (The Storage Reset & Fresh Start Patch):**
  - Web: Implemented browser IndexedDB storage reset via `Module.resetSavedData()` in `web/pre.js`. Added "RESET SAVED DATA" button to launch screen card and in-game top bar (visible when pointer lock is released via <kbd>Esc</kbd> or <kbd>F10</kbd>).
  - Desktop: Added `noctis::reset_runtime_storage()` in `src/runtime_paths.h` and `--reset-data` CLI option in `src/noctis.cpp`, wiping user flight saves, bookmarks, and logs while restoring seed `STARMAP.BIN` and `GUIDE.BIN`.
  - Automated tests: Added `reset_runtime_storage` tests in `tests/runtime_paths_test.cpp` and `tests/runtime_paths_fixture.cmake` (50/50 tests passing across all presets).
  - Packaged Linux, Windows, and Web release archives in `public-builds/2.0.1/` with SHA-256 checksums.
  - Tagged `v2.0.1` and pushed to remote master.

## Test suite and package status

- **Clang Release:** 50/50 passed.
- **GCC Debug:** 50/50 passed.
- **Clang Sanitized (ASan/UBSan):** 50/50 passed with zero errors or memory leaks.
- **MinGW Windows Release:** 50/50 passed; clean `nivlr.exe`.
- **Web Release:** Built cleanly (`nivlr.html`, `nivlr.wasm`, `nivlr.data`).
- **Live Server:** Python 3 daemon serving `build/web-release` at `http://localhost:8090/nivlr.html`.

## Next steps

1. Milestone M13 (Atmospheric Scattering & Horizon Visual Fidelity):
   - M13-W01: Extended surface terrain draw distance & adaptive horizon LOD (configurable terrain rendering radius scaling up to 2x/4x baseline).
   - M13-W02: Twilight atmospheric scattering glow (multi-stop sky scattering gradients during dawn and dusk on worlds with atmospheres).
   - M13-W03: Stellar coronal flare & limb darkening refinement (physically informed limb darkening and procedural coronal prominence flares).
2. Milestone M14: Moviemaker Modernization & Direct Video Export
3. Milestone M15: Ambient Music & Generative Soundscapes
4. Milestone M16: Asynchronous Community GOESnet Catalog Exchange
