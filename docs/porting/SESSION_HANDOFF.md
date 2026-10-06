# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-06.
- Repository: local `Noctis-IV-OM`; branch `master`.
- Active milestone: M17, Engine Consolidation & Runtime Architecture.
- Published checkpoint: M17-W06 in this commit.
- Working tree: clean after the W06 commit.

## Read first

1. `ENGINE_CONSOLIDATION.md` for M17 rules, sequencing, and invariants.
2. `RENDERER_BOUNDS.md` for the W06 memory boundary and sanitizer evidence.
3. M17 in `ROADMAP.md` for work-item status and exit criteria.
4. ADR-0015 in `DECISIONS.md` for the incremental-migration decision.

## M17 completed through W06

- `EngineState` owns transient travel phase and normalized speed.
- GOESnet image exports receive an explicit destination instead of discovering
  host state inside the command layer.
- `ApplicationRuntimeState` enforces legal cockpit, transfer, surface, modal,
  and shutdown transitions; the blocking surface session is RAII-scoped.
- Semantic input recordings are versioned, bounded, atomic, and keyed to fixed
  simulation ticks. The scripted and complete orbit/surface journeys replay at
  different presentation cadences with identical checkpoints.
- The texture mapper now binds each source pointer to its byte capacity while
  retaining legacy 16-bit mask/bias addressing and exact in-range output.
- Textured scanline framebuffer access is bounded to the current active page;
  out-of-range reads return zero and writes are discarded.
- `adapted` is exactly its maximum 1280×800 capacity; `p_surfacemap` is exactly
  its logical 200×200 capacity. Both legacy padding regions are gone.
- FELYSIA crevasse generation bounds every neighbor write independently, which
  removes the border overflow previously hidden by surface-map padding.

## Verification

- Linux Clang Debug: 57/57 tests passed in 30.37 seconds.
- Linux GCC Debug: 57/57 tests passed in 27.36 seconds.
- Clang ASan/UBSan: 57/57 tests passed in 95.39 seconds with leak detection
  disabled because LeakSanitizer cannot run under the desktop runner's ptrace
  policy.
- MinGW Windows Release and Emscripten Web Release both compile and link.
- Canonical renderer, surface generation, live environment/content, scripted
  journey, and orbit/surface hashes and counters remain unchanged.

## Next step

Begin M17-W07 by identifying one coherent save-backed state aggregate whose
capture, restore, live mutation, and fixture coverage can move together. Define
an adapter that preserves native v1 and legacy import bytes before replacing
the corresponding globals; do not split authority between old and new state.
