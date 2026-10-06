# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-06.
- Repository: local `Noctis-IV-OM`; branch `master`.
- Active milestone: M17, Engine Consolidation & Runtime Architecture.
- Published checkpoint: `77ca434` (`refactor(engine): begin M17 runtime consolidation`).
- Working tree: M17-W04 application-mode changes are present and uncommitted.

## Read first

1. `ENGINE_CONSOLIDATION.md` for M17 rules, sequencing, and invariants.
2. M17 in `ROADMAP.md` for work-item status and exit criteria.
3. ADR-0015 in `DECISIONS.md` for the incremental-migration decision.

## Published foundation

- `EngineState` owns transient travel phase and normalized speed.
- GOESnet image-export destinations are injected and tests remain inside their
  fixture workspace.
- M16 online work is deferred until M17; offline catalog exchange remains
  supported.
- Foundation verification before commit: 56/56 Linux Clang Debug tests passed.

## M17-W04 completed locally

- Added `ApplicationMode` values for cockpit, descent/ascent transfer, surface,
  gallery, movie player, and shutdown.
- Added a legal-transition graph with rejected-transition immutability, a
  transition counter, reset semantics, and terminal shutdown.
- Wrapped `planetary_main()` in an application-mode scope. Early returns cannot
  strand the runtime outside the cockpit.
- Routed restored surfaces, fixture surfaces, touchdown, and capsule ascent
  through explicit modes.
- Extended the landing-return fixture to require the four-transition round trip
  back to cockpit.
- Routed gallery and movie-player open/close ownership through `EngineState`.
- Routed normal application teardown into `shutting_down`.

## Verification

- Linux Clang Debug build: passed without warnings.
- Focused application-mode, movie-player, landing, surface, and orbit journey
  suite: 8/8 passed.
- Full Linux Clang Debug lane: 56/56 passed in 22.16 seconds.
- Exact renderer and orbit/surface hashes remain unchanged.

## Next step

Begin M17-W05 by defining a versioned, platform-neutral input-record format
around semantic `InputFrame` values and simulation tick numbers. First replay
the existing scripted interstellar/local journey at multiple presentation
cadences; only then extend recording across the blocking surface session.
