# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-06.
- Repository: local `Noctis-IV-OM`; branch `master`.
- Active milestone: M17, Engine Consolidation & Runtime Architecture.
- Published checkpoint: M17-W05 in this commit.
- Working tree: clean after the W05 commit.

## Read first

1. `ENGINE_CONSOLIDATION.md` for M17 rules, sequencing, and invariants.
2. `INPUT_RECORDING.md` for the semantic recording format and replay contract.
3. M17 in `ROADMAP.md` for work-item status and exit criteria.
4. ADR-0015 in `DECISIONS.md` for the incremental-migration decision.

## M17 completed through W05

- `EngineState` owns transient travel phase and normalized speed.
- GOESnet image exports receive an explicit destination instead of discovering
  host state inside the command layer.
- `ApplicationRuntimeState` enforces legal cockpit, transfer, surface, modal,
  and shutdown transitions; the blocking surface session is RAII-scoped.
- Semantic input recordings use explicit little-endian version 1 storage,
  stable flag assignments, finite float bits, bounded counts, strictly
  increasing simulation ticks, and atomic publication.
- `InputReplay` returns neutral frames at unrecorded ticks and latches skipped
  or out-of-order event delivery as a fixture failure.
- The scripted space journey serializes and decodes its input before replay at
  1x and 4x presentation cadence without changing exact checkpoints.
- The live orbit-to-surface fixture records 783 frames, then replays them in a
  separately seeded process at 4x presentation cadence. All simulation,
  indexed-frame, content, and restored-position evidence agrees.

## Verification

- Linux Clang Debug build completed without warnings.
- Full Linux Clang Debug lane: 57/57 tests passed in 28.47 seconds.
- Linux GCC Debug focused codec, scripted journey, and orbit/surface replay:
  3/3 passed.
- Canonical scripted-journey and orbit/surface state and indexed-frame hashes
  remain unchanged.
- Normal cadence presents 817 orbit/surface frames; the 4x replay cadence
  presents 205 while retaining identical checkpoints.

## Next step

Begin M17-W06 by inventorying every compatibility pad and unbounded renderer or
surface-map write. Add focused boundary tests before changing storage, then
remove padding only where sanitizer and exact fixture evidence prove the new
bounded operation preserves behavior.
