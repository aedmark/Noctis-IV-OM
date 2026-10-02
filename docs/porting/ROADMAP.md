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
| M6 | Noctis IV Plus feature migration | DONE |
| M7 | Cross-platform compatibility preview | IN PROGRESS |
| M8 | Stabilization and 1.0 | IN PROGRESS |

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
