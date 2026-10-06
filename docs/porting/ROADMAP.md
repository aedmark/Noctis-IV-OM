# Modern C++ Port Roadmap

## How to use this roadmap

Milestones describe demonstrable capabilities, not calendar promises. Work may
move within a milestone, but exit criteria should change only through a recorded
decision. The current milestone is the earliest milestone not marked `DONE`.

Work item identifiers use `M<milestone>-W<two digits>`. New items keep their
identifier even if reordered. Dependencies name other work items when relevant.

## Summary

| Milestone | Capability | Status |
| --- | --- | --- |
| M0 | Foundation, provenance, and baseline selection | DONE |
| M1 | Reproducible native build and test shell | DONE |
| M2 | Deterministic galaxy and system compatibility | DONE |
| M3 | Playable space-flight vertical slice | DONE |
| M4 | Planet generation, landing, and surface exploration | DONE |
| M5 | Ship interface, persistence, and GOESnet parity | DONE |
| M7 | Cross-platform compatibility preview | DONE |
| M8 | Stabilization and 1.0 General Availability | DONE |
| M9 | Modern Presentation & Display Enhancements | DONE |
| M10 | Deterministic Upscaling & Fidelity Increase | DONE |
| M11 | In-Engine Media Export & Exploration Ergonomics | DONE |
| M12 | Celestial Cartography & Waypoint Navigation | DONE |
| M13 | Atmospheric Scattering & Horizon Visual Fidelity | DONE |
| M14 | Moviemaker Modernization & Direct Video Export | DONE |
| M15 | Ambient Music & Generative Soundscapes | DONE |
| M16 | Asynchronous Community GOESnet Catalog Exchange | DEFERRED — after M17 |
| M17 | Engine Consolidation & Runtime Architecture | IN PROGRESS |

## M0 — Foundation, provenance, and baseline selection

**Goal:** Begin implementation from a legally and technically understood LR
baseline with a measurable definition of Noctis IV Plus compatibility.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M0-W01 | Establish blueprint, roadmap, decision log, and handoff format | DONE | `docs/porting/` |
| M0-W02 | Inventory Noctis IV LR at an exact commit | DONE | `LR_BASELINE_AUDIT.md`; Clang/GCC builds, startup, and UBSan findings at `e1b0817` |
| M0-W03 | Choose repository/history integration strategy | DONE | ADR-0003; unsquashed LR subtree in `modern/`, tested in a disposable clone |
| M0-W04 | Record license, permission, attribution, code, and asset provenance | DONE | `PROVENANCE.md`; creator-endorsed precedent and ADR-0005 LR compliance assumption recorded; new non-LR material tracked separately |
| M0-W05 | Pin DOS reference executable, data set, DOSBox config, and checksums | DONE | `DOS_REFERENCE.md`; checked-in files at `5c46de9`, no new binaries or runtime fixture claimed |
| M0-W06 | Build NIV+ feature ledger against LR | DONE | 25-row final-state ledger; source/history audit through pinned NIV+, exact LR references, ADR-0008 scope decisions |
| M0-W07 | Define initial compatibility fixture set and capture procedure | DONE | `COMPATIBILITY_FIXTURES.md`; six scenarios, exact initial object IDs, and isolated capture procedure; captures belong to implementation milestones |
| M0-W08 | Record compiler/language/dependency baseline | DONE | ADR-0006; C++20 Clang/GCC syntax checks, exact Raylib/GLM/CPM pins, Windows unverified |

**Exit criteria**

- The LR baseline is identified by repository URL and immutable commit ID.
- A clean build of that baseline has been reproduced or every blocker is named.
- Repository integration, toolchain, and dependency choices are recorded.
- Code and asset provenance is documented, including unresolved permission risk.
- A versioned NIV+ feature ledger exists.
- The first compatibility fixtures and their capture procedure are specified.

## M1 — Reproducible native build and test shell

**Goal:** Every contributor can build, test, and launch the same native baseline.

| ID | Work item | Status | Dependencies |
| --- | --- | --- | --- |
| M1-W01 | Import or establish the LR-derived source tree with attribution | DONE | Unsquashed `modern/` import; split returns exact `e1b0817`, notices/history retained |
| M1-W02 | Normalize CMake presets for supported development platforms | DONE | Linux Clang/GCC C++20 presets build; Windows verification deferred to M7-W01 by ADR-0009 |
| M1-W03 | Pin Raylib, GLM, and other dependencies | DONE | Exact commits and CPM hash; checksum-verified offline bootstrap override |
| M1-W04 | Add unit/fixture test targets and CTest integration | DONE | Headless PRNG, native star/galaxy, and four DOS-reference CTests pass Clang/GCC |
| M1-W05 | Add Linux CI build, tests, and sanitizers | DONE | Root workflow Clang, GCC, and Clang ASan/UBSan jobs passed in [first hosted run](https://github.com/aedmark/Noctis-IV-OM/actions/runs/36248031296) |
| M1-W06 | Add structured logging and a diagnostic startup report | DONE | JSON diagnostics/error-path CTest plus clean five-second host-display ASan/UBSan startup after fixing three reproduced UBs |
| M1-W07 | Document clean build, test, and run workflows | DONE | `modern/BUILDING.md` covers Linux build/test/run, diagnostics, and stale-cache recovery; Windows documentation deferred by ADR-0009 |

**Exit criteria**

- A clean Linux checkout configures, builds, tests, and launches using documented
  commands.
- Dependency revisions are deterministic.
- CI performs the same core workflow and retains useful failure output.
- AddressSanitizer and UndefinedBehaviorSanitizer can run on a smoke path.

## M2 — Deterministic galaxy and system compatibility

**Goal:** Protect the identity of the Noctis universe before expanding gameplay.

| ID | Work item | Status | Dependencies |
| --- | --- | --- | --- |
| M2-W01 | Isolate and document legacy PRNG behavior | DONE | Both production PRNGs have fixed-sequence tests; F01/F02 DOS matches protect observable call order |
| M2-W02 | Replace compatibility-sensitive implicit-width types | DONE | `NUMERIC_WIDTH_AUDIT.md`; explicit seed conversions, fixed-width generator state, safe coordinate arithmetic, and unaligned little-endian reads |
| M2-W03 | Implement galaxy/star fixture runner | DONE | Native matrix covers all 12 star classes, coordinate signs/zero, rarity acceptance/rejection, and all three empty-axis filters; F01A–F01D DOS fields were captured twice and match |
| M2-W04 | Implement system/planet-property fixture runner | DONE | Headless system runner matches F02B DOS identity/type/radius/revolution and F02C's exact landing/global surface seeds; F02C records replayed DOS landing, palette, and terrain evidence |
| M2-W05 | Audit floating-point and iteration-order sensitivity | DONE | `FLOATING_POINT_AUDIT.md`; strict FP contraction policy, association-boundary star, and seven exact full-system fingerprints agree across Clang/GCC/sanitizers |
| M2-W06 | Publish compatibility report for fixture set | DONE | `M2_COMPATIBILITY_REPORT.md` records exact/tolerant coverage, verified toolchains, and assigned limits |

**Exit criteria**

- Selected stars and systems match exact or documented tolerant properties on
  all CI compilers.
- PRNG behavior and compatibility-sensitive arithmetic have focused tests.
- Every known mismatch is fixed, explicitly accepted, or scheduled with an ID.

## M3 — Playable space-flight vertical slice

**Goal:** Launch, inhabit the Stardrifter, navigate a known system, and render it
through a stable native loop.

| ID | Work item | Status |
| --- | --- | --- |
| M3-W01 | Separate deterministic simulation ticks from presentation timing | DONE — fixed 55 ms simulation clock, independent steady-clock presentation pacer, and focused CTest; see `TIMING_MODEL.md` |
| M3-W02 | Establish input action mapping for keyboard and mouse | DONE — injectable semantic `InputFrame`, isolated Raylib provider, preserved legacy key/mouse adapter, and headless CTest; see `INPUT_MAPPING.md` |
| M3-W03 | Preserve indexed-framebuffer and palette presentation path | DONE — pure VGA palette and indexed-to-RGBA stages with headless CTest; Raylib limited to upload/scale; see `INDEXED_PRESENTATION.md` |
| M3-W04 | Verify flat-shaded and textured space rendering fixtures | DONE — exact full-frame hashes from production `poly3d`/`polymap` agree across Clang/GCC/sanitizers; see `SPACE_RENDER_FIXTURES.md` |
| M3-W05 | Complete local and interstellar travel path for a known fixture | DONE — production guidance completes the F02 parent-star/FELYSIA journey with exact cross-compiler tick, arrival, and power checks; see `TRAVEL_PATH.md` |
| M3-W06 | Add scripted smoke journey with state and visual checkpoints | DONE — injected FCS commands complete a 793-tick journey with four exact state/indexed-frame checkpoints invariant across presentation cadence; see `SCRIPTED_JOURNEY.md` |

**Exit criteria**

- A scripted journey can launch, select a target, travel, and arrive without
  manual intervention or sanitizer errors.
- Simulation results are stable across supported frame rates.
- Canonical space views meet their selected comparison levels.

## M4 — Planet generation, landing, and surface exploration

**Goal:** Complete the defining orbit-to-surface exploration loop.

| ID | Work item | Status |
| --- | --- | --- |
| M4-W01 | Characterize planet textures, height maps, and surface sectors | DONE — headless production FELYSIA sector pins exact cross-compiler elevation, texture, and object-map hashes after sanitizer-guided width/bounds repairs; see `SURFACE_GENERATION.md` |
| M4-W02 | Verify generation for representative planet classes | DONE — all seven landable surface families have exact full-buffer native fixtures across Clang/GCC/sanitizers; rocky-branch smoothing bounds and legacy crater coordinate wrapping repaired; see `SURFACE_GENERATION.md` |
| M4-W03 | Complete landing and return-to-ship transitions | DONE — headless production capsule journey lands, walks away/returns, lifts, and restores the exact ship position without a resume file; see `LANDING_TRANSITIONS.md` |
| M4-W04 | Complete terrain, sky, atmosphere, water, and weather paths | DONE — five exact live-frame fixtures cover rainy plains, open water/waves, polar ice, dense atmosphere, and airless rock across both compilers and sanitizers; see `SURFACE_ENVIRONMENT.md` |
| M4-W05 | Complete representative flora, fauna, ruins, and object placement | DONE — exact three-frame native fixtures prove live tree, rock, animal, ruin, and capsule draw paths; packed model storage is aligned and bounded across both compilers and sanitizers; see `SURFACE_CONTENT.md` |
| M4-W06 | Add orbit-to-surface scripted compatibility journey | DONE — one headless production journey requests FELYSIA landing from orbit, renders descent and populated exploration, walks away/returns, lifts, and pins four exact native frames plus state/content counts across compilers and sanitizers; see `ORBIT_SURFACE_JOURNEY.md` |

**Exit criteria**

- Representative worlds for every mandatory planet class generate without
  invalid memory access and meet their compatibility levels.
- A player can land, explore, return, take off, save, and resume.

## M5 — Ship interface, persistence, and GOESnet parity

**Goal:** Restore the complete exploration workflow around the procedural world.

| ID | Work item | Status |
| --- | --- | --- |
| M5-W01 | Specify and implement versioned native saves | DONE — `current.niv` and `surface.niv` v1 have explicit little-endian schemas, CRC-32, bounded reads, replacement writes, native-first startup, and codec/application round-trip tests; see `NATIVE_SAVE_FORMAT.md` |
| M5-W02 | Implement bounded legacy save import | DONE — exact source-backed 245/370/377–382-byte situation and 40/45-byte surface layouts import through validated temporary state and migrate to native v1; see `LEGACY_SAVE_IMPORT.md` |
| M5-W03 | Complete ship screens, HUD, controls, and preferences | DONE — all baseline cockpit routes are inventoried; bounded GOES paging, printable controls, menu/HUD routing, four preferences, and native persistence have focused plus application fixtures; see `SHIP_INTERFACE.md` |
| M5-W04 | Specify GOESnet command and result behavior | DONE — bounded grammar, native registry, fixed-row results, typed status/actions, scope, and failure/atomicity rules have a focused contract test; see `GOESNET_PROTOCOL.md` |
| M5-W05 | Replace mandatory DOS modules with native commands/components | DONE — bounded starmap/guide components implement the required console registry, typed targeting, atomic catalog changes, in-memory output, native-only saves, and an application fixture with no shell or interchange files; see `GOESNET_NATIVE.md` |
| M5-W06 | Validate starmap, guide, catalog, and label workflows | DONE — production application workflows cover help/failures, DOS-backed lookup fields, remote/local targeting, P15 counts, normal UI labels, catalog mutation/reopen/protection, and no-partial-change failures; see `GOESNET_WORKFLOWS.md` |
| M5-W07 | Add corruption, round-trip, and migration tests | DONE — every native envelope byte, checksum-valid semantic corruption, failed replacement/migration, all legacy-to-v1 chains, and a two-process gameplay save/reload journey are covered; see `PERSISTENCE_HARDENING.md` |

**Exit criteria**

- Native saves round-trip and upgrade through a tested schema path.
- Supported legacy state imports with documented outcomes and no unsafe reads.
- Mandatory GOESnet workflows no longer launch DOS executables.

## M6 — Noctis IV Plus feature migration

**Goal:** Bring the modern baseline from Noctis IV compatibility to the agreed
Noctis IV Plus feature set.

M0-W06 supplies the detailed feature ledger. The work packages below are
containers, not substitutes for that ledger.

| ID | Work item | Status |
| --- | --- | --- |
| M6-W01 | Port mandatory NIV+ bug fixes | DONE — panorama temporaries no longer collide with numbered snapshots; label entry suppresses `m`/`s`/`p` actions; shared triad rollover/padding covers Epoc 6012; P15 retains M5 evidence and P18 was subsequently closed by M7-W01; see `NIVPLUS_BUG_FIXES.md` |
| M6-W02 | Port mandatory controls and quality-of-life behavior | DONE — source-backed snapshot/raw/panorama aliases, persisted eight-digit numbering, roof speed, shared three-mode mouselook, jump/jetpack controls, and safely enabled persistent Omega Drive have focused plus application evidence; see `NIVPLUS_CONTROLS.md` |
| M6-W03 | Port mandatory HUD, visor, lens flare, and presentation options | DONE — target radius, extended surface status, depth-255 object viewfield, contextual F1 help, and persisted F2 HUD/flaring/border options have focused application and frame evidence; see `NIVPLUS_PRESENTATION.md` |
| M6-W04 | Migrate supported starmap/guide content and tooling | DONE — the accepted October 2023 archive has a machine-readable manifest and verifier/stager; build initialization and a focused fixture prove missing-only copies preserve player catalogs; see `NIVPLUS_CONTENT.md` |
| M6-W05 | Review deferred NIV+ features and document omissions | DONE — all 25 ledger rows were reconciled; ADR-0013 made Moviemaker core, the removed Tab toggle remains superseded, and P18 was subsequently closed by M7-W01; see `NIVPLUS_SCOPE_REVIEW.md` |
| M6-W06 | Port Moviemaker image-sequence capture in space and on surfaces | DONE — the F3 panel drives safe numbered decks, deterministic cadence, flash modes, record/pause/stop, landing continuity, and the ascent cutoff; focused state/input tests and a production BMP workflow pass all Linux lanes; see `NIVPLUS_MOVIEMAKER.md` |

**Exit criteria**

- Every feature-ledger item is `DONE`, `DEFERRED`, or excluded by a recorded
  decision.
- Mandatory features have automated or reproducible manual acceptance evidence.
- Attribution and asset provenance are complete for migrated work.

## M7 — Cross-platform compatibility preview

**Goal:** Produce installable preview builds suitable for community testing.

| ID | Work item | Status |
| --- | --- | --- |
| M7-W01 | Add and stabilize Windows MSVC CI | DONE — hosted Windows 2022 builds with MSVC and passes 34 platform-independent tests; the portable Release build is confirmed on native Windows and its exact executable passes diagnostics plus the three-frame graphical smoke under Wine; four presentation comparisons remain separately assigned to M7-W05 |
| M7-W02 | Verify Linux packaging on a clean supported distribution | DONE — fresh Ubuntu 24.04 CI builds the release archive, passes all 38 tests, verifies its checksum and exact extracted contents, runs packaged diagnostics plus a software-rendered graphical smoke, and uploads the artifact; see `LINUX_PACKAGING.md` and run `36795944060` |
| M7-W03 | Produce portable Windows package | DONE — fresh Windows 2022 CI builds a static-runtime Release ZIP, passes 34 required tests, verifies checksum and exact extracted contents, runs packaged diagnostics, rejects Visual C++ Redistributable DLL dependencies, and uploads the artifact; see `WINDOWS_PACKAGING.md` and run `36796703199` |
| M7-W04 | Add configuration, data-path, and migration UX | DONE — OS-native Linux/Windows roots, executable-relative immutable resources/defaults, safe explicit migration, read-only path diagnostics, isolated fixtures, and both revised packages pass hosted verification; see `RUNTIME_PATHS.md` and runs `36866928901`, `36866929363`, `36866929200`, and `36866929028` |
| M7-W05 | Run cross-platform fixture suite and publish known differences | DONE — the report publishes the compiler-specific surface boundary; exact Linux and MSVC Release baselines are enforced, all 40 Windows Release tests pass, and `KNOWN_ISSUES.md` ships in both packages; see `M7_COMPATIBILITY_REPORT.md` and run `36869013306` |
| M7-W06 | Conduct focused community compatibility test | IN PROGRESS — candidate pair v0.0.1-preview released; maintainer manual core route on Linux confirmed working; recruitment for Windows testing in progress |

**Exit criteria**

- Linux and Windows artifacts are built from CI and run from clean environments.
- The compatibility report and known-issues list ship with the preview.
- Crash reports and diagnostics contain enough context for actionable triage.

## M8 — Stabilization and 1.0

**Goal:** Turn the compatibility preview into a maintainable stable release.

| ID | Work item | Status |
| --- | --- | --- |
| M8-W01 | Resolve release-blocking compatibility and stability defects | DONE — resolved lithium depletion bug on relaunch; hardened 2D flare line drawer with viewport clipping against 1-3 byte buffer overflows; fixed operator precedence in additive polygon flare blending; prevented font glyph buffer underflow and restored row 0 rendering; guarded NOMINMAX macro; replaced non-portable gcvt with std::snprintf; added bounds safety check in single_pixel_at_ptr; all 40 tests pass across all configurations |
| M8-W02 | Complete user, contributor, migration, and troubleshooting docs | DONE — created CONTRIBUTING.md and comprehensive TROUBLESHOOTING.md; updated PACKAGE_README.md, PACKAGE_README_WINDOWS.md, README.md, and BUILDING.md; packaged and verified TROUBLESHOOTING.md in release archives |
| M8-W03 | Establish performance budgets and profile representative scenes | DONE — established performance and memory budgets in docs/porting/PERFORMANCE_BUDGETS.md; measured 1.98 ms/frame software rasterization (96.4% CPU headroom under 55 ms budget) and 7.2 MB headless / 92 MB graphical peak RSS |
| M8-W04 | Verify license notices, provenance, and distribution contents | DONE — verified WTOF-LICENSE.md, LICENSE, CONTRIBUTORS.md, THIRD_PARTY_NOTICES.md, and licenses/; enforced exact distribution manifests via VerifyLinuxPackage.cmake and VerifyWindowsPackage.cmake |
| M8-W05 | Rehearse release and rollback from a clean tag | DONE — created docs/porting/RELEASE_RUNBOOK.md; successfully executed complete packaging rehearsal, checksum verification, isolated archive extraction, preflight diagnostics, and graphical smoke testing |
| M8-W06 | Tag and publish 1.0 with checksums and release notes | DONE — tagged v1.0.0, built verified Linux and Windows 1.0 release packages, generated SHA-256 checksums, published public-builds/1.0.0/ and release notes |

**Exit criteria**

- All mandatory blueprint success criteria have evidence.
- No open release-blocking defect or unresolved distribution gate remains.
- A clean tag reproducibly yields tested release artifacts.

## M9 — Modern Presentation & Display Enhancements

**Goal:** Deliver modern, flexible, high-fidelity windowing and display presentation while strictly preserving retro 3D software rendering and determinism.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M9-W01 | Aspect ratio preservation & dynamic letterboxing | DONE | Implemented `calculate_viewport()` with authentic 4:3 CRT proportions (canonical round celestial bodies, dynamic pillarboxing/letterboxing in arbitrary aspect ratios), 16:10 square-pixel mode, and 16:9 stretch mode; cycle with `F8`; verified in `tests/display_test.cpp` |
| M9-W02 | Window management, resizing, and fullscreen | DONE | Updated window title to "Noctis IV OM"; enabled dynamic window resizing via `FLAG_WINDOW_RESIZABLE` with minimum 640x480 bounds; added `F11` and `Alt+Enter` toggle for true/borderless fullscreen; verified in `tests/input_mapping_test.cpp` |
| M9-W03 | High-DPI HUD overlay | DONE | Implemented `render_high_dpi_hud()` rendering transient telemetry, FCS status messages, and display mode notifications at full display resolution above the retro canvas with frosted glass badge, glowing status pip, and smooth fadeout |

**Exit criteria**

- Window can be resized to arbitrary resolutions without distorting celestial body geometry.
- Fullscreen and aspect ratio cycling operate dynamically via hotkeys and display high-DPI status badges.
- All existing compatibility fixtures and determinism test suites pass with identical hashes.

## M10 — Deterministic Upscaling & Fidelity Increase

**Goal:** Provide advanced deterministic post-processing, upscaling, and geometry fidelity enhancements to elevate visual presentation on modern high-resolution displays.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M10-W01 | Deterministic edge-directed upscaling | DONE | Implemented Scale2x / EPX deterministic edge-directed filter expanding 320x200 to 640x400 with diagonal smoothing and 100% color/palette preservation; cycle with `F7` or `F2` menu; verified in `tests/upscale_test.cpp` |
| M10-W02 | CRT simulation shader pipeline | DONE | Implemented OpenGL 3.3 GLSL CRT post-processing shader with aperture grille phosphor triads, 200-line scanlines, gentle glass curvature, corner vignette, and star core bloom; toggle with `F6` or `F2` menu; verified in `tests/display_test.cpp` |
| M10-W03 | Sub-pixel geometry rasterization & fidelity mode | DONE | Implemented sub-pixel geometry rasterization across `poly3d` (continuous barycentric half-pixel sampling) and `polymap` (floating-point vertex projection, sub-pixel edge pre-stepping and scanline rounding eliminating 3D mesh wobble and jitter) plus bilinear celestial point distribution in `far_pixel_at`; toggle with `F2` menu (`G`); preserved byte-exact legacy mode by default with 100% fixture agreement in `tests/renderer_fixture_test.cpp` |
| M10-W04 | Display & presentation settings persistence | DONE | Implemented user configuration persistence in `display_settings.ini` within platform `config_dir`; automatically saves and reloads aspect ratio mode, upscale mode, CRT shader, sub-pixel fidelity, fullscreen, timewarp multiplier, HUD text, lens flare mode, and seamless border; verified in `tests/display_test.cpp` |

**Exit criteria**

- Upscaling filters produce deterministic results across platforms without visual artifacts or tearing.
- CRT shaders run within a strict 1.0 ms GPU budget.
- Sub-pixel fidelity eliminates geometric jitter on textured and flat 3D surfaces and celestial bodies.
- Display and presentation preferences persist across game sessions in the user configuration directory.
- Legacy software rendering remains available as a toggleable baseline, with 100% regression fixture agreement.

## M11 — In-Engine Media Export & Exploration Ergonomics

**Goal:** Provide seamless in-engine media export for the browser and desktop editions, accompanied by input and exploration ergonomic enhancements.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M11-W01 | In-engine screenshot & panorama download/export | DONE | Direct in-engine export for captured snapshots and panoramas in the F4 Image Archive Viewer (key `D`/`P` or Export button, format toggle `F` for PNG/BMP: browser file download in Web via Emscripten JavaScript bridge, system Downloads export on desktop), headless CLI image export (`--export-image`), GOESnet terminal `VIEW EXPORT`, and automatic PNG export to Downloads upon capture (`M` snapshot, `N` panorama); verified in `tests/gallery_test.cpp` and passing CTest suites |
| M11-W02 | Flight & maneuvering acoustics | DONE | Procedural sublight RCS attitude thrusters (onset cold-gas valve burst + continuous bandpass hiss during attitude changes), atmospheric descent buffeting turbulence scaled with entry velocity and air density, and dual-stage mechanical touchdown clunk on planetary landing impact/bounce; verified in `tests/audio_test.cpp` and 47/47 passing CTest suites |
| M11-W03 | Gamepad and joystick flight controls | DONE | Dual-stick flight and surface traversal via Raylib Gamepad API (analog yaw/pitch/roll, trigger thrusters, context-sensitive B-button cancellation, dual-motor rumble feedback); verified in `tests/gamepad_test.cpp` and 47/47 passing CTest suites |
| M11-W04 | Configurable controls & sensitivity persistence | DONE | User-configurable keybindings, mouse sensitivity sliders, mouse pitch inversion (push-forward = look-up default), gamepad deadzones, and settings persistence in `controls.ini`; verified in `tests/controls_test.cpp` and 47/47 passing CTest suites |
| M11-W05 | Audio category volume controls | DONE | Implemented 5 independent volume categories (Master, Spaceflight & RCS, Cockpit Foley, Visor & Suit, Environment & Surface) with High-DPI graphics/audio menu overlay (`F2` or `Tab`/`A`), mouse drag/click and keyboard controls (`+`/`-`, `1`–`5`, `M`), and persistence in `config.ini`; verified in `tests/audio_test.cpp` and 47/47 passing CTest suites |
| M11-W06 | Cockpit & GOESnet tactile foley | DONE | Tactile procedural dashboard rocker switches and console buttons, 3-variation mechanical vintage terminal keystroke clacks, telemetry transmit chirps, dual-harmonic acknowledge bell / error buzz, linefeed scroll taps, and observation deck elevator carriage servo; verified in `tests/audio_test.cpp` and 47/47 passing CTest suites |

**Exit criteria**

- Web players can download captured screenshots and panoramas directly to their computer (as PNG or BMP) with a single in-engine keypress or button in the F4 Image Archive without opening developer tools.
- Desktop players can open the gallery folder, export captures directly as PNGs to Downloads, or automatically export PNGs on every snapshot/panorama capture.
- All existing 47 automated test suites continue passing with 100% determinism.

## M12 — Celestial Cartography & Waypoint Navigation

**Goal:** Provide comprehensive navigational and logbook tools for deep exploration, including an automated captain's flight journal, custom star and planet bookmarks with quick-nav targeting, and a toggleable surface exploration HUD with compass heading and coordinates.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M12-W01 | In-engine captain's flight log | DONE | Automated flight log recording visited systems with light-year jump distances, orbital arrivals, surface landings with lat/long coordinates, and star/planet naming events; GOESnet `LOG`/`JOURNAL` 21-column pager, `LOG EXPORT` (Markdown `flight_log.md` and JSON `flight_log.json`), and `NAME`/`LABEL` GOESnet commands writing to `STARMAP.BIN`; verified in `tests/flight_log_test.cpp`, `tests/goesnet_commands_test.cpp`, and passing test suites |
| M12-W02 | Starmap bookmarks & waypoint navigation | DONE | Save and manage labeled star/planet bookmarks with quick-target recall in the navigation computer and starmap; GOESnet `BM`/`BOOKMARK`/`WAYPOINT` pager, `BM ADD`/`GOTO`/`DEL`/`CLEAR`, cockpit `J` quick-jump, surface `J` instant GPS waypoint drop, persisted in `bookmarks.ini`; verified in `tests/bookmarks_test.cpp` and `tests/goesnet_commands_test.cpp` |
| M12-W03 | Surface & orbital navigation HUD | DONE | The Explorer's Visor HUD featuring 360° cardinal compass tape, digital heading, planetary lat/lon coordinates, elevation MSL in meters, and real-time Lander Return Beacon bearing arrow and range; surface `V` mode toggle with suit servo audio feedback; verified in `tests/navigation_hud_test.cpp` and 50/50 passing test suites across all compilers and sanitizers |

**Exit criteria**

- Visited systems and planetfalls are automatically journaled in an in-engine log with browsable UI and export capability.
- Players can bookmark stars and planets, assign custom notes/labels, and quickly lock guidance to bookmarked coordinates.
- Surface explorers can toggle a compass and coordinate display to orient themselves and locate previous landing sites or points of interest.
- All test suites pass with 100% determinism.

## M13 — Atmospheric Scattering & Horizon Visual Fidelity

**Goal:** Elevate planetary and stellar visual fidelity with expanded horizon terrain draw distance, dynamic twilight atmospheric scattering gradients, and refined stellar coronal flares.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M13-W01 | Extended surface terrain draw distance & adaptive horizon LOD | DONE | Configurable terrain rendering radius scaling up to 96Q (1.5x) and 128Q (2x) with adaptive far-quad polygon mesh construction, horizon sky sealing, sub-pixel edge pre-stepping, and full fixture determinism; toggle with `D` in F2 Visual menu or `--draw-distance`; verified in `tests/display_test.cpp` |
| M13-W02 | Twilight atmospheric scattering glow | DONE | Multi-stop Rayleigh and Mie twilight sky scattering, dynamic twilight arch and Belt of Venus in panoramic sky maps, horizon sky glow, and solar disk limb attenuation during civil/nautical twilight; toggle with `S` in F2 Visual menu (`AUTHENTIC`, `REALISTIC`, `VIBRANT`) or `--atmospheric-scattering`; verified in `tests/atmospheric_scattering_test.cpp` |
| M13-W03 | Spectral color fidelity & dynamic coronal flares | DONE | Eddington quadratic limb darkening (1 - u*(1-mu) - v*(1-mu)^2) for 3D volumetric incandescence, procedural multi-harmonic coronal streamer flares, pulsar relativistic twin-jets, flare star CME eruptive loops, and Planckian radiation ramps across all 12 stellar classes in space (`white_globe`) and planetary skies (`white_sun`); toggle with `E` in F2 Visual menu or `--coronal-flares`; verified in `tests/stellar_coronal_flares_test.cpp` and 53 passing CTest suites |

**Exit criteria**

- Extended horizon draw distance can be toggled without frame drops or visual artifacts.
- Sunrise and sunset on atmospheric worlds exhibit natural twilight sky glow transitions.
- All test suites pass with 100% determinism.

## M14 — Moviemaker Modernization & Direct Video Export

**Goal:** Streamline media capture with direct video encoding and in-cockpit replay.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M14-W01 | In-browser WebM video recording | DONE | Direct canvas capture and video download via browser `MediaRecorder` API in Emscripten bridge (`start_browser_video_recording`, `stop_browser_video_recording`), synchronized with F3 Moviemaker capture cycle without external frame stitching; verified in `src/video_export.cpp` and `src/noctis.cpp` |
| M14-W02 | Desktop automated MP4/WebM video export | DONE | Automated in-engine export invoking system/bundled FFmpeg (`-c:v libx264` / `-c:v libvpx-vp9`), non-blocking background worker thread (`export_movie_deck_async`), direct placement into user's Downloads directory, CLI export flags (`--export-movie <deck>`, `--export-fps`, `--export-out`), and GOESnet terminal `MOVIE EXPORT <deck>`; verified in `tests/video_export_test.cpp` and `tests/goesnet_commands_test.cpp` |
| M14-W03 | In-cockpit Moviemaker deck preview | DONE | In-cockpit high-DPI projector viewer with CRT framing, playback engine (Play/Pause, Step, Home/End, Loop, Framerate cycling 6-60 FPS), interactive draggable timeline scrubber bar, export button, F3 Moviemaker panel shortcuts (`V` for preview, `X` for export), and GOESnet `MOVIE [PLAY <deck>]`; verified in `tests/movie_player_test.cpp` and 55 passing test suites |

**Exit criteria**

- Web players can record and export playable WebM videos directly from the browser.
- Desktop players can generate encoded video files with one click or command.
- Recorded decks can be previewed inside the ship.
- All test suites pass with 100% determinism.

## M15 — Ambient Music & Generative Soundscapes

**Goal:** Integrate atmospheric exploration music tracks and procedural generative ambient layers beneath physical acoustics.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M15-W01 | Authentic ambient exploration music integration | DONE | Multi-format playlist scanner and audio stream player (`src/music.h`, `src/music.cpp`) supporting `.ogg`, `.mp3`, `.wav`, `.flac`, `.xm`, `.mod`, auto-progression, next/prev skip (`N`/`P`), and graceful headless fallback; verified in `tests/audio_test.cpp` |
| M15-W02 | Procedural generative ambient melody & chord synthesizer | DONE | 4-voice procedural generative synthesizer in `audio_stream_callback` (`src/audio.cpp`): foundation bass pad, stereo chorused chord pad (14s progression), generative melodic chime with slow scaling modal arpeggios (2.4s steps, bell envelope decay, ping-pong pan), celestial glass shimmer, 12 stellar modal scales, pulsar tremolo, and noise-free clean audio bus |
| M15-W03 | Music volume controls & playlist preferences overlay | DONE | Dedicated 6th audio channel `MUSIC` in F2 menu and high-DPI overlay (`src/display.cpp`), playback mode toggle (`G` key / interactive button: `GENERATIVE`, `RECORDED`, `HYBRID`, `OFF`), and persistent storage in `config.ini` / `audio_settings.ini`; verified in `tests/audio_test.cpp` and `tests/plus_presentation_test.cpp` |

**Exit criteria**

- Music tracks and generative drones play smoothly through miniaudio without blocking the main loop or clipping.
- Music volume is fully controllable and persistent in `config.ini`.
- All test suites pass with 100% determinism.

## M16 — Asynchronous Community GOESnet Catalog Exchange

**Goal:** Connect explorers across platforms with optional cloud/community star catalog synchronization.

M16-W01 and M16-W02 are deferred until M17 establishes explicit runtime and
platform-service boundaries. The offline exchange layer remains supported.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M16-W01 | Community catalog sync protocol | PLANNED | Lightweight REST/WebSocket protocol for exchanging star names, planetary annotations, and exploration logs |
| M16-W02 | Web and desktop community catalog client | PLANNED | In-engine sync toggle and background updater fetching recent community discoveries |
| M16-W03 | Conflict resolution & offline-first queue | DONE | Non-destructive merging of local and remote discoveries, binary `.nsm` / JSON packet format, CRC32 verification, canonical seed protection, ID/name collision resolution, atomic replacement, `OUTBOX`/`INBOX`/`CLEAN` GOESnet commands, CLI import/export/validate, and web browser download/upload bridge; verified in `tests/starmap_exchange_test.cpp` and 51/51 passing test suites |

**Exit criteria**

- Players can optionally synchronize catalog entries with a community server.
- Local catalogs are never corrupted or overwritten destructively.
- All test suites pass with 100% determinism.

## M17 — Engine Consolidation & Runtime Architecture

**Goal:** Give the completed engine an explicit state root, testable platform
boundaries, and staged application lifecycle without changing universe identity
or player-visible behavior.

| ID | Work item | Status | Evidence/notes |
| --- | --- | --- | --- |
| M17-W01 | Record consolidation architecture and invariants | DONE | ADR-0015 and `ENGINE_CONSOLIDATION.md` |
| M17-W02 | Establish application-owned engine state | DONE | `EngineState` owns transient travel phase/speed; live travel and audio telemetry migrated; focused state test |
| M17-W03 | Inject filesystem/export platform boundaries | DONE | GOESnet image exports receive an explicit destination; fixture writes remain inside the test workspace |
| M17-W04 | Introduce explicit application mode transitions | DONE | Tested legal-transition graph for cockpit, descent/ascent, surface, gallery, movie player, and shutdown; production surface-session RAII prevents stranded modes on early return; landing fixture asserts the four-transition round trip; modal viewers own open/close transitions |
| M17-W05 | Add deterministic input recording and replay | DONE | Versioned, bounded semantic `InputFrame` format; byte-stable codec and strict replay cursor; scripted space journey exact at 1×/4× presentation; recorded 783-frame orbit-to-surface session replays from a clean profile at 4× presentation with identical simulation, indexed-frame, and content checkpoints; see `INPUT_RECORDING.md` |
| M17-W06 | Bound renderer and surface-map memory access | PLANNED | Remove compatibility padding only after exact fixture and sanitizer evidence |
| M17-W07 | Migrate coherent save-backed state aggregates | PLANNED | Explicit adapters preserve native v1 and legacy import schemas |

**Exit criteria**

- New engine state has one explicit owner and no new cross-file mutable globals.
- Platform filesystem, clock, presentation, audio, and process services are
  replaceable in headless tests.
- Application modes transition through a testable state machine.
- A recorded journey replays to identical simulation and indexed-frame
  checkpoints at multiple presentation cadences.
- Known renderer overrun padding is replaced by bounded operations with
  sanitizer and exact-fixture evidence.
- Linux Clang/GCC, sanitizers, Windows, and Web retain their supported gates.

## Work item template

Use this template when a roadmap item needs decomposition in an issue or task:

```markdown
### M?-W?? — Short capability name

Status: NOT STARTED
Owner: unassigned
Depends on: IDs or none
Decision records: ADR IDs or none

Outcome:
One observable result, not a list of coding activities.

Acceptance:
- [ ] Objective check
- [ ] Relevant automated test or documented manual procedure
- [ ] Documentation and handoff updated

Notes:
Constraints, provenance, fixture IDs, and links.
```
