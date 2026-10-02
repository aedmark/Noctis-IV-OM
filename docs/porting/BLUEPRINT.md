# Modern C++ Port Blueprint

## 1. Mission

Create a native, maintainable continuation of Noctis IV Plus that runs on
modern desktop operating systems without DOSBox while preserving the identity
of the original universe.

The project will take the baton from
[Noctis IV LR](https://github.com/dgcole/noctis-iv-lr). We will reuse and extend
its modern C++ work where licensing and technical review permit, then migrate
the Noctis IV Plus behavior and content that are not already represented there.

The first production targets are 64-bit Linux and Windows. macOS is a supported
design target and becomes a release target after the core is stable.

## 2. What success means

The first stable release must:

- build with a documented CMake workflow on supported 64-bit platforms;
- run without a DOS, BIOS, or 16-bit compatibility layer;
- generate the same galaxy identities and materially equivalent stellar and
  planetary properties for the agreed compatibility fixture set;
- preserve the quiet exploration loop, ship operation, landing, surface
  exploration, saving, and loading;
- read project-owned legacy data needed for continuity, with explicit error
  messages for unsupported or corrupt input;
- provide deterministic simulation behavior for a fixed seed and configuration;
- include the selected Noctis IV Plus fixes and quality-of-life features;
- keep platform, simulation, rendering, and persistence concerns separable;
- ship with automated smoke, serialization, and procedural-compatibility tests.

Pixel-identical rendering is not a default requirement. Deterministic universe
identity and recognizable generated worlds are higher priorities. Any intended
deviation from legacy behavior must be documented and testable.

## 3. Non-goals for the first stable release

- A wholesale cleanup or redesign before behavioral parity exists.
- A new game with altered progression, combat, or authored objectives.
- Networked multiplayer or a central online service.
- Perfect binary compatibility with every community-modified save file.
- Reproducing undefined behavior, memory corruption, or hardware timing bugs
  unless players depend upon the resulting behavior.
- A browser build. The architecture must not prevent one, but desktop parity
  comes first.
- Rewriting working Noctis IV LR systems solely to impose a preferred style.

## 4. Source-of-truth hierarchy

When sources disagree, use this order and record significant exceptions:

1. An explicit project decision in `DECISIONS.md`.
2. A captured behavior fixture from the agreed Noctis IV Plus DOS reference
   build.
3. Noctis IV Plus source and data in this repository.
4. Noctis IV LR behavior and source.
5. Original Noctis IV documentation and source comments.
6. Maintainer recollection or visual intuition.

This order lets us intentionally improve behavior while preventing accidental
drift from masquerading as modernization.

## 5. Foundation and repository strategy

Before importing code, milestone M0 must inventory Noctis IV LR and record:

- repository URL and exact upstream commit;
- license and redistribution conditions for code and assets;
- buildable platforms and toolchain versions;
- implemented and incomplete subsystems;
- test coverage and known compatibility gaps;
- differences from the Noctis IV Plus source and data set.

ADR-0003–ADR-0005 select an unsquashed Git subtree import of LR under `modern/` in this
repository. The native build root will be `modern/`. The legacy `source/`,
`modules/`, `data/`, and `manual/` trees remain comparison material. M1-W01
may perform the import after M0-W04's provenance inventory. Per ADR-0005, LR
content is assumed license-compliant; attribution and any new material added
from outside LR remain subject to `PROVENANCE.md`.

## 6. Proposed runtime architecture

The target is conventional C++20 with CMake, accepted by ADR-0006 after syntax
checks of the inherited LR sources with Clang 22 and GCC 16. Full C++20 builds
and Windows compiler validation remain M1/M7 work.
Use fixed-width integer types at file-format and compatibility boundaries.

```text
Application
  owns lifecycle, state transitions, configuration, and error reporting
      |
      +-- Platform
      |     window, input, timing, paths, logging, process-independent I/O
      |
      +-- Simulation
      |     galaxy, systems, flight, ship, surface gameplay, deterministic clock
      |
      +-- Generation
      |     legacy PRNG, stars, planets, terrain, textures, flora/fauna placement
      |
      +-- Rendering
      |     scene extraction, software/legacy-compatible paths, modern GPU output
      |
      +-- Persistence
      |     native saves, legacy import, starmap/guide formats, resource loading
      |
      +-- Interface
            ship controls, HUD, GOESnet experience, menus, accessibility
```

### 6.1 Dependency direction

- Simulation and generation may depend on small math and compatibility
  libraries, but not on windowing, GPU, wall-clock, or operating-system APIs.
- Rendering consumes immutable or short-lived scene/state views; it must not own
  simulation truth.
- Platform services are injected through narrow interfaces where determinism or
  testing matters.
- Persistence translates explicit disk schemas to runtime types. Runtime struct
  layout is never itself a file format.
- UI issues commands to the application/simulation layer rather than mutating
  unrelated global state directly.

These boundaries may initially wrap inherited global-state code. They are
directional seams, not a demand for an immediate rewrite.

### 6.2 Platform and graphics layer

Noctis IV LR currently uses Raylib and GLM; retain those dependencies initially
unless M0 finds a concrete blocker. A framework migration before parity would
multiply risk without improving compatibility.

Preserve the legacy 320x200 indexed-color presentation as a first-class render
mode:

- render or resolve to an 8-bit logical framebuffer and 256-entry palette where
  legacy effects depend upon palette semantics;
- upload the resolved image to a modern texture for scaling and presentation;
- use integer scaling when possible, with configurable aspect correction;
- isolate enhanced-resolution or GPU-native rendering behind a separate mode.

This gives us a dependable visual reference while leaving room for enhancements.

### 6.3 Determinism contract

Procedural compatibility is a product feature. The following are controlled:

- PRNG algorithm, state width, seeding, call order, and integer overflow rules;
- explicit 8-, 16-, 32-, and 64-bit representations at compatibility seams;
- floating-point evaluation where it changes generated identity or topology;
- simulation tick policy and conversion from wall time;
- sorting tie-breakers and iteration order;
- serialization byte order and schema version.

Do not use the platform C library's `rand()`, unspecified struct packing, or
unordered iteration in deterministic code. When exact legacy floating-point
results are impractical, define tolerances and compare stable derived properties.

### 6.4 Persistence policy

Use two explicit paths:

1. **Legacy import:** read known Noctis IV/IV Plus formats using bounded,
   byte-oriented parsers. Never reinterpret a runtime struct over file bytes.
2. **Native save:** a versioned, documented format with atomic replacement,
   validation, and forward migration.

Legacy files should be treated as untrusted input. Parsers require size checks,
range checks, useful diagnostics, and corruption tests.

### 6.5 GOESnet modules

The DOS version shells out to auxiliary executables and exchanges state through
files. The modern port should preserve the player-facing command environment
while removing dependence on DOS executables.

Preferred order:

1. Define the command/result protocol and observable behaviors.
2. Implement built-in commands behind a command registry.
3. Port valuable auxiliary modules to native library components or standalone
   portable tools only when process isolation is useful.
4. Consider a stable extension interface after first-release parity, not before.

## 7. Compatibility program

### 7.1 Reference environment

Pin a known Noctis IV Plus executable, data set, DOSBox configuration, and
starting save. Record checksums rather than committing files whose distribution
rights are unclear.

### 7.2 Fixture classes

- **Galaxy fixtures:** coordinates, identity, class, name, planet count, and
  target calculations for selected systems and boundary values.
- **Planet fixtures:** physical properties, moon relationships, palette, texture
  or height-map hashes, and selected surface sectors.
- **Simulation fixtures:** scripted input over fixed ticks with state snapshots.
- **Persistence fixtures:** legacy saves, round trips, truncated files, oversized
  fields, and cross-version upgrades.
- **Visual fixtures:** canonical scenes captured at the logical framebuffer,
  compared exactly where appropriate and perceptually elsewhere.
- **Workflow fixtures:** launch, new session, travel, land, return, save, reload,
  and GOESnet commands.

Every fixture needs provenance: reference version, input, capture method, and the
property it protects.

### 7.3 Comparison levels

- `EXACT`: byte-, integer-, or hash-identical.
- `TOLERANT`: numerical result within a documented absolute/relative tolerance.
- `STRUCTURAL`: same topology, classification, ordering, or count.
- `PERCEPTUAL`: approved image threshold plus human review when it changes.
- `INTENTIONAL DIFFERENCE`: linked decision record and a test for the new rule.

## 8. Engineering workflow

### 8.1 Small vertical slices

Port one observable path at a time. A preferred early slice is:

1. start application;
2. load resources;
3. generate a known system;
4. render one canonical exterior view;
5. accept input for a fixed number of ticks;
6. save and reload state;
7. verify the state and framebuffer fixture.

This exposes integration problems earlier than porting entire subsystems in
isolation.

### 8.2 Change rules

- Separate compatibility changes from optional enhancements.
- Add characterization tests before correcting unclear inherited behavior.
- Keep imported code mechanically close to upstream until protected by tests.
- Replace magic binary offsets with named schemas and assertions.
- Prefer warnings-as-errors for first-party code; quarantine inherited warnings
  temporarily with a tracked removal plan.
- Sanitizers must run regularly on a supported development platform.
- A change is not `DONE` until documentation and the session handoff agree with
  the repository.

### 8.3 Proposed quality gates

- Configure and build with Clang and GCC on Linux, MSVC on Windows.
- Unit and fixture tests through CTest.
- ClangFormat for first-party C++ and CMake formatting conventions.
- AddressSanitizer and UndefinedBehaviorSanitizer jobs on Linux.
- Static analysis after the baseline import is stable.
- Reproducible dependency acquisition with pinned revisions.
- Release artifacts built by CI from a clean checkout.

## 9. Feature migration policy

Create a feature ledger during M0 with one row per meaningful Noctis IV Plus
change. Each row must identify:

- player-visible behavior;
- source commit or source-code location;
- required data/assets;
- whether LR already implements it;
- compatibility fixture or manual test;
- target milestone and status;
- permission or provenance concern, if any.

Classify features as `bug fix`, `compatibility`, `quality of life`, `content`, or
`enhancement`. First-release priority is compatibility and high-value bug fixes;
new enhancements wait until the baseline is trustworthy.

## 10. Risk register

| Risk | Consequence | Response |
| --- | --- | --- |
| Modification/distribution permission is unclear | Work cannot be released safely | Make provenance and permission an M0 release gate; retain notices and attribution |
| LR diverges from current NIV+ behavior | Hidden regressions during feature migration | Build a feature ledger and DOS-backed compatibility fixtures |
| Legacy floating-point and PRNG behavior drift | Different galaxy or planetary terrain | Isolate compatibility math and test known seeds on every platform |
| Inherited global state resists modularization | Slow changes and fragile tests | Introduce seams around vertical slices; avoid a big-bang rewrite |
| Binary formats depend on Borland widths/packing | Save corruption or silent incompatibility | Byte-oriented parsers, fixed-width types, golden files, fuzz/corruption tests |
| Renderer modernization changes the visual identity | Port feels unlike Noctis | Preserve indexed framebuffer/palette mode before enhanced rendering |
| Scope expands into a remake | Parity never reaches a releasable state | Enforce milestone exit criteria and defer enhancements |
| Upstream becomes active or changes direction | Forks duplicate effort | Keep provenance, an upstream remote, and an explicit sync policy |

## 11. Release definitions

### Bring-up build

Developers can build and enter one deterministic space scene on Linux. It may
be incomplete and is not a user release.

### Compatibility preview

Core travel, generation, landing, saving, and representative NIV+ features work
on Linux and Windows. Known differences are documented.

### 1.0

All mandatory roadmap capabilities pass their exit criteria, supported-platform
artifacts are produced from CI, migration and troubleshooting are documented,
and licensing/provenance gates are satisfied.

## 12. Immediate decisions

M0 must resolve these before broad implementation:

1. Which exact Noctis IV LR commit is the baseline?
2. Does this repository become the port repository, or will the modern port live
   in a new/forked repository with this one retained as a reference?
3. What written permission covers modification and distribution of LR, NIV+, and
   their assets?
4. Which Noctis IV Plus build and data set are the behavioral reference?
5. Is C++20 acceptable for every first-release platform?
6. Which NIV+ features are mandatory for the first compatibility preview?

Answers belong in numbered decision records, not only in a session handoff.
