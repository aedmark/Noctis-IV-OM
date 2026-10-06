# Modern C++ Port Documentation

This directory is the control center for the modern C++ port of Noctis IV
Plus. The intended starting point is Noctis IV LR, not a line-by-line
conversion of the Borland C++ code in `source/`.

## Document map

| Document | Purpose | Update cadence |
| --- | --- | --- |
| [BLUEPRINT.md](BLUEPRINT.md) | Product boundaries, architecture, compatibility policy, and engineering method | Only when the durable plan changes |
| [ROADMAP.md](ROADMAP.md) | Milestones, work packages, exit criteria, and current status | Whenever work changes milestone status |
| [LR_BASELINE_AUDIT.md](LR_BASELINE_AUDIT.md) | Pinned LR source, build, dependencies, subsystem state, and gaps | When the candidate upstream commit changes |
| [PROVENANCE.md](PROVENANCE.md) | Code, asset, license, attribution, and permission evidence/gates | Whenever rights evidence or package scope changes |
| [DOS_REFERENCE.md](DOS_REFERENCE.md) | Pinned DOS executable, data, configuration, and checksums | When the canonical DOS reference changes |
| [NIVPLUS_FEATURE_LEDGER.md](NIVPLUS_FEATURE_LEDGER.md) | Player-visible NIV+ changes versus LR, proposed scope, and acceptance evidence | As parity is verified or scope decisions change |
| [COMPATIBILITY_FIXTURES.md](COMPATIBILITY_FIXTURES.md) | Initial DOS fixture scenarios, manifest schema, and isolated capture procedure | As captures are performed or comparison policy changes |
| [FLOATING_POINT_AUDIT.md](FLOATING_POINT_AUDIT.md) | M2 floating-point grouping, PRNG iteration order, compiler controls, and regression evidence | When compatibility arithmetic or toolchain policy changes |
| [SHIP_INTERFACE.md](SHIP_INTERFACE.md) | M5-W03 cockpit screens, HUD routes, controls, preferences, repairs, and fixture evidence | When ship-interface behavior or bindings change |
| [GOESNET_PROTOCOL.md](GOESNET_PROTOCOL.md) | M5-W04 native command grammar, registry, result shape, failure behavior, and W05/W06 handoff | When GOESnet commands, data semantics, or dispatch behavior change |
| [GOESNET_NATIVE.md](GOESNET_NATIVE.md) | M5-W05 native data readers, command components, shell/interchange removal, and focused evidence | When native GOESnet implementation or data mutation changes |
| [GOESNET_WORKFLOWS.md](GOESNET_WORKFLOWS.md) | M5-W06 end-to-end starmap, targeting, guide, catalog, and normal UI label evidence | When a GOESnet player workflow or its acceptance evidence changes |
| [PERSISTENCE_HARDENING.md](PERSISTENCE_HARDENING.md) | M5-W07 corruption matrix, atomic-failure, upgrade-chain, and gameplay restart evidence | When save validation, replacement, or migration behavior changes |
| [NIVPLUS_BUG_FIXES.md](NIVPLUS_BUG_FIXES.md) | M6-W01 mandatory NIV+ bug-fix mapping, implementation, deferral, and acceptance evidence | When a mandatory NIV+ fix or its evidence changes |
| [NIVPLUS_CONTROLS.md](NIVPLUS_CONTROLS.md) | M6-W02 snapshot, mouselook, roof-speed, surface-movement, and Omega Drive behavior/evidence | When NIV+ controls or their acceptance evidence changes |
| [NIVPLUS_PRESENTATION.md](NIVPLUS_PRESENTATION.md) | M6-W03 radius, surface status, extended viewfield, F1 help, and F2 visual behavior/evidence | When NIV+ presentation or its acceptance evidence changes |
| [NIVPLUS_CONTENT.md](NIVPLUS_CONTENT.md) | M6-W04 pinned starmap/guide manifest, staging policy, provenance, and package evidence | When bundled catalog content or packaging changes |
| [NIVPLUS_SCOPE_REVIEW.md](NIVPLUS_SCOPE_REVIEW.md) | M6-W05 final ledger disposition, reopened Moviemaker scope, and exclusions | When first-release Plus scope changes |
| [NIVPLUS_MOVIEMAKER.md](NIVPLUS_MOVIEMAKER.md) | M6-W06 pinned controls, native design boundary, safety policy, and acceptance plan | During Moviemaker implementation |
| [WINDOWS_MSVC.md](WINDOWS_MSVC.md) | M7-W01 hosted MSVC build/test evidence, portability fixes, and remaining real-machine gate | During Windows compiler and startup validation |
| [LINUX_PACKAGING.md](LINUX_PACKAGING.md) | M7-W02 Linux archive contract, runtime boundary, verification, and clean-runner evidence | When Linux package contents or support claims change |
| [WINDOWS_PACKAGING.md](WINDOWS_PACKAGING.md) | M7-W03 Windows ZIP contract, static runtime boundary, verification, and real-PC handoff | When Windows package contents or validation changes |
| [RUNTIME_PATHS.md](RUNTIME_PATHS.md) | M7-W04 OS paths, immutable defaults, profile creation, migration safety, and evidence | When runtime storage or migration changes |
| [M7_COMPATIBILITY_REPORT.md](M7_COMPATIBILITY_REPORT.md) | M7-W05 Linux/MSVC presentation comparison, exact platform baselines, and accepted differences | When a cross-platform fixture or baseline changes |
| [COMMUNITY_COMPATIBILITY_TEST.md](COMMUNITY_COMPATIBILITY_TEST.md) | M7-W06 launch gates, coverage target, triage policy, and closure evidence | During the focused community test |
| [SESSION_HANDOFF.md](SESSION_HANDOFF.md) | Exact state needed to resume work in the next session | At the end of every working session |
| [DECISIONS.md](DECISIONS.md) | Numbered architectural and project decisions | When a consequential choice is made or superseded |
| [ENGINE_CONSOLIDATION.md](ENGINE_CONSOLIDATION.md) | M17 state ownership, platform seams, migration order, and invariants | During engine consolidation work |
| [INPUT_RECORDING.md](INPUT_RECORDING.md) | M17-W05 semantic input format, validation, replay rules, and journey evidence | When input recording or replay behavior changes |
| [RENDERER_BOUNDS.md](RENDERER_BOUNDS.md) | M17-W06 framebuffer/texture bounds, removed padding, surface repair, and sanitizer evidence | When renderer or surface-map memory access changes |
| [SAVE_STATE_AGGREGATES.md](SAVE_STATE_AGGREGATES.md) | M17-W07 engine-owned persistent aggregates, flat-schema adapters, and byte-compatibility evidence | When save-backed runtime ownership changes |

The original DOS sources and executable remain the behavioral reference. They
are not expected to become the production build.

## Operating rules

1. Start each session by reading `SESSION_HANDOFF.md`, then the active milestone
   in `ROADMAP.md`. Read the full blueprint when the task changes architecture
   or scope.
2. Give roadmap work a stable identifier such as `M1-W03`. Use that identifier
   in commits, handoffs, and pull requests where practical.
3. Update tests and compatibility fixtures with behavior changes. A visual
   impression alone is not sufficient evidence of procedural compatibility.
4. Record decisions that constrain later work in `DECISIONS.md`. Routine
   implementation choices do not need a decision record.
5. End every session by replacing `SESSION_HANDOFF.md` with current facts. Do
   not leave speculative work under “Completed.” Git history is the archive.
6. Keep the handoff concise enough to read in five minutes. Link to code,
   decisions, or test output instead of pasting long logs.

## Status vocabulary

- `NOT STARTED`: no implementation work has begun.
- `IN PROGRESS`: actively being implemented; the handoff names the next step.
- `BLOCKED`: cannot advance without a named decision, dependency, or permission.
- `IN REVIEW`: implementation is complete but its exit criteria are not yet met.
- `DONE`: every listed exit criterion has objective evidence.
- `DEFERRED`: intentionally removed from the current delivery path.

Percent-complete estimates are deliberately avoided. A milestone is measured by
its exit criteria and verified capabilities.
