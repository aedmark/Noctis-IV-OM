# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-01.
- Repository: local `Noctis-IV-Plus`, remote project `aedmark/Noctis-IV-OM`.
- Branch: `master`; M7-W01–W05 are complete and M7-W06 is in progress.
- Focus: M7-W06 manual validation withdrew the `6617d0a` candidate pair after
  exposing unreadable cockpit flicker and ambiguous terminal/flight guidance.
  Follow-up testing of the `7e764da` pair distinguished the remaining blink as
  the inherited zero-power warning and exposed a broken depleted-save recovery
  option. A new candidate pair is required after that fix. Recruitment remains
  closed pending complete maintainer routes on both.
- Public-facing progress is now tracked in `../../devlog.html`. Update it when
  closing a work item or whenever the plain-English project story materially changes.

## Read first

1. `ROADMAP.md` for statuses/dependencies.
2. `../../modern/BUILDING.md` for Linux build/test/diagnostics and cache recovery.
3. `DECISIONS.md`, especially ADR-0005–0009 and ADR-0013.
4. `COMPATIBILITY_FIXTURES.md` and fixture manifests for DOS evidence.
5. `NIVPLUS_FEATURE_LEDGER.md` for the finalized first-release parity inventory.
6. `../../devlog.html` for the public, plain-English development record.

## Completed this session

- Implemented M7-W04's runtime-path boundary. Linux uses XDG data/config roots
  with HOME fallbacks; Windows uses Local/Roaming AppData Known Folders.
  Resources and immutable catalog seeds resolve beside the executable under
  `res/` and `defaults/`, while saves, mutable catalogs, screenshots, and movie
  decks use the player profile regardless of working directory.
- Added a headless `--prepare-user-data`, explicit `--user-data-dir`, and safe
  `--migrate-from` workflow. Migration copies only recognized missing regular
  files, recursively merges gallery/movie output, skips links, preserves every
  collision, never deletes the source, and is idempotent. Read-only diagnostics
  now report every resolved path. ADR-0014 and `RUNTIME_PATHS.md` record the
  contract.
- Revised both package contracts: archives contain immutable `defaults/`
  instead of writable colocated output directories, and their verifiers create
  an isolated player profile, seed both catalogs, and require all four mutable
  directories. Both revised archives pass their hosted package verifiers.
- Closed M7-W04 at commit `2f0a1b2`. Visual Studio's multi-configuration build
  initially placed resources one level above `nivlr.exe`; the post-build step
  now targets the executable directory. The same validation exposed an inherited
  surface-rendering out-of-bounds read after low power cleared the orbital
  target. Retaining the landed body's type removes the undefined behavior and
  restores deterministic GCC/sanitizer fixtures. Hosted Linux build/package
  runs `36866928901`/`36866929363` and Windows build/package runs
  `36866929200`/`36866929028` are green.
- Closed M7-W05. The report distinguishes exact semantic/structural agreement
  from compiler-specific surface buffers and indexed frames. Linux and MSVC
  Release have separate exact manifests; the shipped Windows lane now gates all
  40 non-graphical tests. `KNOWN_ISSUES.md` ships in both archives. Windows run
  `36869013306` and Linux runs `36869013216`/`36869013197` pass at `6617d0a`.
- Pinned the W06 candidate pair from `6617d0a`: Linux SHA-256
  `f1397a195e44b94f29d8dc188f29fc7d70defa29424c2474b8fec5772c625b35`
  and Windows SHA-256
  `75b54d47c7a68610f6bef06623255bbfed45f2e8241eef72289357017f651a06`.
  Gates 1–3 are satisfied; the manual core route is still required before
  recruitment.
- Began M7-W06 preparation without falsely opening or closing the external
  test. `modern/COMMUNITY_TESTING.md` gives players a 20–30 minute core route,
  safe optional migration route, diagnostic capture commands, blocker policy,
  and report link. A GitHub issue form collects exact build, environment,
  diagnostics, checklist, and reproduction evidence. The internal campaign
  plan defines minimum Linux/Windows machine coverage, triage, retest rules,
  and explicit M7-W04/W05 launch gates. Both package contracts now require the
  tester guide.
- The first hands-on Linux route used the exact `6617d0a` archive on CachyOS,
  KDE Wayland through XWayland, and an AMD Radeon RX 7800 XT. Startup, keyboard,
  mouse, F1 help, and F2 visual settings passed. Testing then exposed a rapid
  inherited brightness pulse on uppercase cockpit text and an unclear guide
  step: `7` is terminal input while GOESnet is selected, not a global flight
  command. The pair is withdrawn before recruitment.
- Removed the per-tick cockpit text pulse, added focused regression coverage,
  documented the required GOESnet deselection plus `5`, `7` flight sequence,
  and corrected the false audio expectation: this preview has no game-audio
  playback path. All 40 tests pass under Clang, GCC, and Clang ASan/UBSan. A
  verified local Release archive with SHA-256
  `b007539e71eb3af044c5c5424de45430ac2c110c822cc74a17f91a4b529cae12`
  passed a focused manual retest: text was steady and Vimana flight started.
  It is diagnostic evidence, not a distributable campaign candidate.
- Pinned the corrected W06 candidate pair from `7e764da`. Linux run
  `36873954692` produced SHA-256
  `843c8729d079d28e0ac805a71d3225ecab14171c1e2a20fde7b72c08a43e59c9`;
  Windows run `36873954600` produced SHA-256
  `586d016d4e51dee23035bc8b711b74f0141fd448659b4b4a7ab1c348a4a4da05`.
  Both included checksum files pass independent verification. Linux passed all
  40 tests, package verification, and graphical smoke; Windows passed all 40
  non-graphical Release tests, package verification, and static-runtime checks.
- Reproduced the apparent post-restart flicker from the exact Linux candidate,
  executable, and persisted profile. Frame captures proved that the onboard
  terminal was periodically omitted, while decoding the native save identified
  the cause: exactly 15,000 power and zero lithium reserves activate the
  inherited power-loss warning. This was neither an old binary nor corrupted
  persistence. The investigation did expose a real recovery defect:
  `--standard-drive` only disabled Omega and did nothing for a depleted standard
  drive. It now restores 20,000 power and all 120 lithium charges, with focused
  coverage and an explicit tester-guide explanation. The original profile and
  all diagnostic material remain under ignored `modern/build/w06-testing/`, not
  a temporary directory. The `7e764da` pair is superseded; build and pin a fresh
  same-commit pair before resuming the maintainer route.

- M0-W06 is complete. The ledger now has 25 final-state rows with exact NIV+
  and LR source/history evidence. It adds space mouselook, raw panorama, and
  the F2 visual-effects menu from post-2.3 history. It records the removed Tab
  binding as superseded rather than current behavior.
- The CE audit found no confirmed active delta for the changelog's named
  exponential-pressure, replacement-surface, or hopper-highlighting claims;
  they were not invented as requirements. ADR-0008 keeps Omega Drive and
  extended viewfield core and originally deferred Moviemaker; ADR-0013 now
  restores Moviemaker to first-release scope. M0 is DONE.
- Reproduced three current startup UBs in a disposable five-second host-display
  Clang ASan/UBSan run: unaligned glyph-word load, out-of-range float-to-byte
  conversion, and out-of-range session seed conversion.
- Fixed those cases with bytewise little-endian reads and explicit modulo-width
  conversion. Corrected the session-ID format mismatch and four integer `fabs`
  warnings. The repeated graphical smoke reached the active loop cleanly.
- Began M2-W02 with shared legacy numeric helpers and a focused CTest. Audited
  floating inputs to both PRNG seed functions across the active modules and
  made their 16/32-bit signed and unsigned modulo behavior explicit. Reworked
  the star-coordinate seed expression to retain its left-to-right `% 10000`
  behavior without signed overflow. Broader type auditing remains open.
- Closed M2-W03. The native fixtures now cover all 12 star classes, all sector
  coordinate signs including zero, rarity acceptance/rejection, and the three
  empty-axis filters. Four separately repeated DOS star captures remain the
  external compatibility evidence; native-only cases are labeled as such.
- Closed M2-W04. F02B's repeated DOS identity, type, radius, environment, and
  revolution match the native generator using the legacy `4*pi/3` mass factor.
  F02C lands on FELYSIA at `LQ 001:060`, records its green vegetated surface,
  replays its serialized state from a fresh root, and matches native global
  surface seed `952631` and local landing seed `60`.
- Closed M2-W05. `FLOATING_POINT_AUDIT.md` records every extracted M2 numeric
  boundary and its evidence. Compatibility targets disable FMA contraction;
  the star matrix includes a regrouping-sensitive identity, and the system
  matrix fingerprints every field of seven complete systems (up to 70 bodies)
  in generation order. Clang, GCC, and sanitized Clang agree exactly.
- Closed M2-W01 and M2-W02. Both production PRNGs are isolated and have
  fixed-sequence tests. `NUMERIC_WIDTH_AUDIT.md` records all extracted M2 width
  boundaries, including the final star-identity and star-face seed conversions.
- Closed M2-W06 and M2. `M2_COMPATIBILITY_REPORT.md` publishes the exact DOS
  matches, broader native matrix, comparison levels, and assigned scope limits.
- Closed M3-W01. Simulation now advances by an integer-counted fixed 55 ms
  clock after startup synchronization; rendering and double-click timing use
  `steady_clock`. CPU-clock seeds, FPS-derived fractions, and busy waits are
  gone. `TIMING_MODEL.md` and ADR-0010 record the boundary and follow-on limits.
- Closed M3-W02. All live Raylib input polling is isolated behind an injectable
  semantic `InputFrame`. The compatibility adapter preserves WASD state, mouse
  scaling/buttons, text filtering, and DOS extended-key ordering; focused tests
  cover direct mapping and a synthetic provider without a window.
- Closed M3-W03. VGA palette filtering and indexed 320×200-to-RGBA expansion
  are pure tested stages; Raylib now only uploads/scales explicit RGBA bytes.
  Channel order, alpha, prefix palette writes, clamp behavior, and 63→252
  intensity are pinned, and the conversion buffer is reused between frames.
- Closed M3-W04. A headless runner exercises the production `poly3d` and
  `polymap` paths with canonical camera-space panels. Exact hashes of all 64,000
  indexed pixels and nonzero counts agree across both compilers and sanitizers.
- Closed M3-W05. Interstellar and local guidance now live in a testable
  production module. The canonical F02 journey reaches BALASTRACKONASTREYA in
  396 ticks and FELYSIA P04 in another 392, including the inherited power and
  lithium recharge behavior.
- Closed M3-W06 and M3. Injected flight-control commands complete the canonical
  journey in 793 simulation ticks. Four exact state/indexed-frame checkpoints
  remain identical when presentation work runs every tick or every fourth tick.
- Closed M4-W01. A headless production probe regenerates FELYSIA at `LQ 001:060`
  and pins the complete elevation, ground-texture, and packed object maps.
  Characterization fixed four modern width/bounds faults before accepting the
  first hashes; all three Linux lanes now agree exactly.
- Closed M4-W02. The probe now covers all seven landable planet families with
  exact native full-buffer baselines. The wider matrix found and repaired a
  40-byte rocky-texture smoothing over-read and three undefined crater/storm
  coordinate conversions; all three Linux lanes agree exactly.
- Closed M4-W03. A headless production journey reaches FELYSIA's ground,
  walks away from and back to the capsule, completes the normal lift, and
  restores the ship exactly. It fixed double-to-float position loss across a
  surface visit and corrected the emergency checkpoint's misplaced fixture-only
  rendering boundary.
- Closed M4-W04. Five deterministic live surface frames cover rainy plains,
  open ocean with both wave paths, polar ice, dense atmosphere, and airless
  rock. The matrix fixed undefined perspective conversion/interpolation and a
  32 KiB wave-texture addressing overflow; Clang, GCC, and ASan/UBSan agree on
  every complete indexed frame.
- Closed M4-W05. Three-frame content fixtures prove the production tree,
  animal, ruin, loose-rock, and capsule draw paths. The pass replaced
  misaligned packed-model float access with aligned native storage, bounded a
  polygon-color loop, and made the capsule's 32 KiB texture wrapping explicit.
- Closed M4-W06 and M4. One headless production journey requests landing from
  FELYSIA orbit, renders the descent and populated world, walks away and back,
  lifts, and restores the ship. Four exact frame checkpoints and all state and
  content counts agree across both compilers and sanitizers.
- Closed M5-W01. `data/current.niv` v1 encodes the full 370-byte application
  prefix plus eleven NIV+ extension bytes as explicit little-endian fields
  inside a versioned, sized, CRC-32 envelope. `surface.niv` does the same for
  emergency exploration checkpoints. Native state is authoritative when
  present, writes use a temporary replacement, and reads are bounded.
- Closed M5-W02. Exact source-backed 245/370/377–382-byte situation layouts
  and 40/45-byte surface layouts decode into validated temporary state before
  application, then migrate to native v1 without deleting the input. The
  tracked 378-byte NIV+ fixture and a DOS-sized surface checkpoint both migrate
  through the real application. Unknown, oversized, damaged, and invalid
  layouts fail without a raw or partial read.
- Closed M5-W03. The baseline Stardrifter interface now has an explicit
  inventory and focused production module. GOES output paging uses the actual
  file length, reads all 147 visible bytes, clamps Home/End/line/page movement,
  and terminates outside the visible cells. Printable ASCII now reaches GOES,
  labels, coordinates, and the `+` cabin-light control without duplicate
  Raylib events. A headless application fixture verifies four top-level menus,
  all five device pages, all three HUD data panels, four preference transitions,
  and preference persistence. Plus-only help, mouselook, roof-speed, visor, and
  visual settings remain assigned to M6 rather than being pulled forward.
- Closed M5-W04. The GOESnet boundary now has a tested 83-character grammar,
  exact command registry and scope, fixed 21-byte output rows, typed result
  status/actions, stable failures, and atomicity rules. W05 must remove shell
  execution and legacy state/COMM exchange; full workflows remain in W06.
- Closed M5-W05. The required console registry now runs as bounded native
  components over validated starmap/guide records. Typed ST actions replace
  COMM.BIN, output pages owned memory instead of GOESfile.txt, guide changes
  use complete replacement writes, normal saves no longer emit current.bin,
  and retired DOS maintenance/exchange commands cannot launch executables.
- Closed M5-W06. A production application fixture now composes help and error
  paths, DOS-backed MIRACLE/FELYSIA lookup, remote/local targeting, P15 planet
  note counts, normal UI star/planet label round trips, and guide catalog
  mutation with reopen/protection checks. Focused tests prove failed writes do
  not partially change data and repair an inherited label removal path that
  could truncate the starmap. `GOESNET_WORKFLOWS.md` pins the P22 checksums.
- Closed M5-W07 and M5. Native situation and surface readers reject a one-bit
  change at every envelope byte plus checksum-valid semantic corruption before
  publishing state. Every supported legacy layout upgrades through native v1;
  blocked and read-only replacements retain the committed/source files. A
  two-process production journey targets BALASTRACKONASTREYA and FELYSIA,
  changes cockpit preferences and a HUD panel, saves, restarts without the
  legacy input, verifies the state, continues, and saves again. See
  `PERSISTENCE_HARDENING.md`.
- Closed M6-W01 on Linux. Panoramic capture now uses an isolated `WIDE`
  temporary namespace and an atomic production composer, so ordinary numbered
  snapshots cannot be overwritten. Snapshot and ship shortcuts are suppressed
  during label entry, and both time displays share the corrected modulo-billion,
  three-digit triad formatting across Epoc 6012. P15 retains its M5 evidence;
  Windows-only P18 was subsequently closed by M7-W01. See
  `NIVPLUS_BUG_FIXES.md`.
- Closed M6-W02 on Linux. Normal/raw snapshot aliases and surface panoramas
  share persisted eight-digit NICE-style numbering; Delete remains non-text.
  Space/surface three-mode mouselook, roof-speed pacing, jump/jetpack controls,
  and Omega Drive recharge are active production behavior. Omega has safe
  enable/disable startup options, and the application restart journey restores
  it with the snapshot, mouselook, and roof-speed state. See
  `NIVPLUS_CONTROLS.md`.
- Closed M6-W03 on Linux. Target sheets include radius, surface status uses the
  extended Plus text, and the full object population remains visible through
  depth 255. Contextual F1 help and the persisted F2 HUD, lens-flare, and visor
  border settings run in space and on surfaces. See `NIVPLUS_PRESENTATION.md`.
- Closed M6-W04 on Linux. The October 2023 starmap and guide now have a
  machine-readable release manifest plus a verifier/stager. Focused package
  evidence checks hashes, layout, exact clean staging, and preservation of
  an existing player catalog. See `NIVPLUS_CONTENT.md`.
- Closed M6-W05's inventory, then reopened M6 by explicit user direction.
  ADR-0013 restores P09 Moviemaker to first-release scope as M6-W06. The other
  22 implemented rows retain their evidence; P18 was subsequently completed in
  M7-W01 and P19 remains superseded by P25. See `NIVPLUS_SCOPE_REVIEW.md`.
- Closed M6-W06 and M6 on Linux. The F3 Moviemaker records safe numbered BMP
  decks in space and on surfaces with deterministic cadence, pause/resume,
  landing continuity, two flash modes, occupied-deck refusal, and the pinned
  ascent cutoff. Focused state/input tests and a headless production workflow
  verify exact files and frames. See `NIVPLUS_MOVIEMAKER.md`.
- Added a responsive project homepage at `index.html` that identifies Noctis IV
  OM up front as a hybrid of Noctis IV Plus and Noctis IV LR, introduces it to
  new players, and provides a five-step first-flight primer, quick-reference
  controls, authentic game imagery, honest development status, and routes to
  the manual, source, and now-standalone styled `devlog.html`. Shared styling
  lives in `site.css`; desktop and narrow layouts were checked in a browser.
- Corrected the Linux dependency contract: all presets explicitly build
  Raylib/GLFW for X11, CI and contributor commands no longer install unused
  Wayland development packages, and the docs distinguish XWayland runtime
  compatibility from an unverified native Wayland backend. Peter Spicer is
  credited for identifying the issue.
- Began M7-W01 without changing the Linux-first development workflow. Added a
  pinned Visual Studio 2022 CMake preset and Windows 2022 GitHub Actions job.
  The first compiler pass exposed and fixed five cross-translation-unit type
  mismatches in inherited text buffers and description tables.
- Fixed Windows replacement semantics for native saves, panoramas, starmap
  labels, and guide edits. A shared same-filesystem boundary uses
  `MoveFileExW` with replace/write-through on Windows and `rename()` on POSIX;
  all affected application and focused tests now pass on both platforms.
- The required Windows lane passes 34/34 platform-independent tests and uploads
  a provisional debug runtime. Evidence is GitHub Actions run `36793746326` at
  `24ed729`; see `WINDOWS_MSVC.md`.
- M7-W02 is complete. A runtime-only Linux archive now carries the game,
  verified catalog seeds, writable output directories, player guidance,
  credits, and license texts without developer headers, source, historical DOS
  binaries, or excluded media. Fresh Ubuntu 24.04 run `36795944060` passed all
  38 release tests, verified the archive/checksum and extracted diagnostics,
  launched the extracted game under Mesa/Xvfb, and uploaded the artifact. See
  `LINUX_PACKAGING.md`.
- M7-W03 is complete. Fresh Windows 2022 run `36796703199` produced the
  runtime-only Release ZIP, passed all 34 required tests, verified its checksum
  and extracted layout, ran packaged diagnostics, proved its mutable folders,
  and rejected Visual C++ Redistributable DLL dependencies before upload. The
  2,217,922-byte artifact was independently downloaded and inspected on Linux.
  See `WINDOWS_PACKAGING.md`.
- Closed M7-W01 and P18 after the maintainer confirmed the portable package on
  native Windows and Wine. The exact MSVC executable (SHA-256 `2551502975bb612bde6ecaca397f5e1f5c44bf8066ba6733aaa725828ad9a1ed`)
  also passed `--diagnostics` and `--graphical-smoke` under Wine, opening the
  Win32 Raylib backend through OpenGL 4.6, presenting three frames, emitting the
  success event, and exiting 0 without input. M7-W05 still owns four distinct
  presentation-hash differences.
- Simplified the build documentation to one canonical Linux quick start.
  Replaced the stale inherited LR/MSYS2 instructions, reduced `BUILDING.md` to
  optional configurations and troubleshooting, and established clear local
  `build/<preset>/` versus `build/packages/` roles. Preserved two local saves
  under ignored `modern/player-data-backup/` before removing obsolete build
  output.
- Four exact MSVC surface/presentation comparisons retain matching semantics,
  counts, and timing with selected compiler-specific buffer/frame hashes.
  M7-W05 publishes and enforces the reviewed Release baselines. The hosted
  Windows VM still lacks an OpenGL driver, but native Windows and Wine startup
  supply the separate M7-W01 evidence.
- ADR-0009's deferred Windows build/startup validation is complete in M7-W01.
  Full cross-platform presentation review is complete in M7-W05.

## Validation

- Clang, GCC, and Clang ASan/UBSan each pass 40/40 tests with the new focused
  runtime-path/migration test. GCC exposed and prompted removal of a collision
  with its predefined `linux` macro. The revised Linux TGZ passes exact-content,
  diagnostics, and isolated-profile preparation verification.
- The M7-W06 preparation rebuild passed 38/38 Clang debug tests. A fresh Linux
  TGZ was generated from that tree and passed the exact extracted-package
  verifier with `COMMUNITY_TESTING.md` present.

- Full rebuild from current source and restored pinned dependencies succeeds
  with Clang 22.1.8, GCC 16.2.1, and Clang ASan/UBSan.
- All 40/40 CTests pass in each current Linux lane. Coverage includes
  Moviemaker state, input, and production BMP workflows; content package
  verification and preservation; Plus controls/presentation; timing,
  input, ship-interface unit
  and application coverage, GOESnet protocol/data/commands/application, indexed presentation,
  exact flat/textured renderer fixtures, the native travel journey, both PRNG
  suites, F02C, the scripted smoke journey, all seven landable surface families,
  landing return, the five live environment frames, two populated surface
  content frames, the complete orbit-to-surface journey, both native-save
  fixtures, and the legacy import matrix. Local
  sanitized CTest used `ASAN_OPTIONS=detect_leaks=0` for the restricted environment; leak detection
  was not validated this session. CI retains leak detection by default.
- Hosted Windows 2022/MSVC Release run `36869013306` configured and built the
  complete project, passed all 40 non-graphical tests including the reviewed
  presentation baselines, verified the ZIP/checksum/profile/static-runtime
  contract, and uploaded the W06 candidate. Debug run `36869013330` retains the
  four presentation tests as a recorded non-blocking diagnostic probe.
- A five-second host-display X11/OpenGL run of the sanitized binary reached the
  active loop with no sanitizer diagnostic and ended by timeout (124). It used
  a disposable `/tmp` runtime with empty data/gallery directories.
- F02B used two fresh DOS roots; F02C used a fresh landing plus a fresh
  `SURFACE.BIN` replay. Screenshots remain outside Git with hashes and exact
  regeneration procedures in their manifests.

## Current compatibility state and limits

- Four DOS-confirmed stars (ADELPHE, JEHOVABOH, MIRACLE, NEW FELYSIA) match native
  coordinates, class, RGB, spin, and radius bits. Broader coverage remains open.
- FELYSIA is P04 of BALASTRACKONASTREYA, not NEW FELYSIA. F02A establishes its
  parent, F02B its repeated properties, and F02C its visitable surface. Native
  M4 coverage now includes deterministic terrain, rendering, populated surface
  exploration, landing transitions, and a normal return to orbit.
- Both PRNGs, compatibility-sensitive seed conversions, and resource-read
  semantics have focused tests. Repeated DOS star/system outcomes protect the
  observable generator call order within M2 scope.
- The complete mandatory GOESnet workflow is now covered headlessly through the
  production application, including combined catalog and normal-label paths.
  This does not claim manual host-window interaction. The
  pinned starmap and guide now initialize missing build runtime files without
  overwriting an existing writable catalog.
  Windows/MSVC verification is deferred to M7 by ADR-0009.
- Diagnostic preflight tests archive readability/non-emptiness, not integrity;
  directory presence, not writability. The clean smoke is not gameplay or
  clean-shutdown evidence. The renderer fixture now reaches the historical
  texture-mapping area cleanly under ASan/UBSan for its selected quad.
- The M3 smoke is headless and supplies deterministic fixture target resolution;
  it does not exercise GOESnet lookup, every interactive cockpit route, a host
  window, or DOS-pixel identity. Its selected native state/frame checks are exact.
- F02C's seeds and structural/perceptual observations remain the DOS evidence;
  M4-W01/W02's full elevation, texture, and object hashes are exact native baselines,
  not extracted DOS buffers.
- M4-W04's complete indexed-frame hashes are also native baselines. The fixture
  omits life, ruins, loose objects, and the capsule so failures remain separate
  from M4-W05's two populated native frame baselines.
- M4-W06 composes those native boundaries into one exact scripted workflow.
  F02C grounds its selected world and surface observations, but the four journey
  frame hashes and timing are not claimed as DOS-exact evidence.
- Types 0, 6, 9, and 10 remain orbital-only by design; live landing controls
  reject volcanic worlds, gas giants, substellar objects, and companion stars.
- Native v1 covers the 370-byte LR situation plus the eleven pinned NIV+
  extension bytes and the full LR surface checkpoint. Exact historical layouts
  migrate through M5-W02, and M5-W07 now covers every-byte/semantic corruption,
  interrupted replacement, all supported upgrade chains, and a gameplay restart.

## Next actions

1. Complete the W06 core route once on each `7e764da` candidate. This is
   hands-on gameplay and requires keyboard/mouse input.
2. Publish the invitation with the replacement hashes, recruit the required Linux
   and Windows machine coverage, triage reports, and require retests for any
   corrected candidate. Do not call W06 complete before real reports satisfy
   its closure matrix.

## Worktree and resume

The replacement W06 implementation checkpoint is `7e764da`; its exact Linux
and Windows artifacts are pinned in `COMMUNITY_COMPATIBILITY_TEST.md`. The old
`6617d0a` pair is withdrawn. No community campaign has launched. Resume with
the complete manual core route on each replacement candidate.
The normal development workflow stays on Linux.
The prior subtree import preserves LR notices and history.

```sh
git status --short
cd modern
cmake --build --preset linux-clang-debug --parallel 2
ctest --preset linux-clang-debug
ctest --preset linux-gcc-debug
ASAN_OPTIONS=detect_leaks=0 ctest --preset linux-clang-sanitized
```

The existing caches now reference persistent dependency sources. If rebuilding
from another checkout, use the normal pinned downloads or the documented offline
overrides; do not reuse the deleted `/tmp` paths from earlier handoffs.
