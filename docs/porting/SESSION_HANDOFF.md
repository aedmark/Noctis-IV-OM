# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-06.
- Repository: local `Noctis-IV-OM`; branch `master` at the v3.0.0 baseline.
- Active milestone: M17, Engine Consolidation & Runtime Architecture.
- Working tree: M17 first-slice changes are present and uncommitted.

## Read first

1. `ENGINE_CONSOLIDATION.md` for M17 rules, sequencing, and invariants.
2. `ROADMAP.md` for work-item status and exit criteria.
3. ADR-0015 in `DECISIONS.md` for the incremental-migration decision.
4. `BLUEPRINT.md` for durable compatibility and dependency-direction policy.

## Completed this session

- Created M17 and deferred the online M16 client/server work until the runtime
  and platform boundaries are ready. Offline `.nsm`/JSON exchange remains
  supported.
- Added the application-owned `noctis::EngineState` root.
- Migrated transient travel phase and normalized travel speed out of
  file-local globals. FCS transitions, remote/local travel advancement, and
  audio telemetry now share explicit begin/update/reset semantics.
- Added `engine_state` focused tests for defaults, transitions, speed clamping,
  and reset behavior.
- Added an injected GOESnet image-export destination. Desktop composition
  supplies Downloads, Web retains its browser bridge, and tests write only
  inside their fixture workspace.
- Added M17 documentation and ADR-0015. Also restored ADR-0014 to the decision
  index.

## Verification

- `cmake --build --preset linux-clang-debug --parallel`: passed.
- Focused engine state, GOESnet, travel, audio, scripted journey, and full
  orbit-to-surface tests: 6/6 passed.
- Full Linux Clang Debug lane: 56/56 passed in 22.34 seconds.
- `git diff --check`: passed before final formatting; rerun before commit.

## Compatibility notes

- No save schema, deterministic generator, travel calculation, simulation tick,
  or renderer algorithm changed.
- The new state root owns only transient travel presentation data. Legacy
  position, guidance, targeting, and save-backed values remain authoritative
  until they can migrate as one coherent aggregate.
- The previous GOESnet test failure under restricted HOME access is eliminated;
  no XDG or HOME override is required.

## Next step

Start M17-W04 with a characterization test for application mode transitions.
Model the current cockpit-to-descent-to-surface-to-cockpit route without first
changing loop control. Then route one existing transition through that model.
Do not migrate save-backed globals or replace the blocking loops in the same
slice.
