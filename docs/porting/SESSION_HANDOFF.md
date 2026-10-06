# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-06.
- Repository: local `Noctis-IV-OM`; branch `master`.
- Active milestone: M17, Engine Consolidation & Runtime Architecture.
- Published checkpoint: M17 exit audit in this commit; milestone complete.
- Working tree: clean after the W07 commit.

## Read first

1. `ENGINE_CONSOLIDATION.md` for M17 rules, sequencing, and invariants.
2. `M17_EXIT_AUDIT.md` for final criterion and verification evidence.
3. M17 in `ROADMAP.md` for work-item status and exit criteria.
4. ADR-0015 in `DECISIONS.md` for the incremental-migration decision.

## M17 complete

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
- `GoesTerminalState`, owned by `EngineState`, is the sole live owner of the
  GOESnet command buffer, command cursor, and output scroll offset.
- Explicit capture/restore adapters preserve the existing flat native v1
  fields and all legacy normalization paths without a schema-version change.
- The exit audit passes all six roadmap criteria with direct code and fixture
  evidence; no deferred M17 blocker remains.

## Verification

- Linux Clang Debug: 57/57 tests passed in 29.61 seconds.
- Linux GCC Debug: 57/57 tests passed in 27.10 seconds.
- Clang ASan/UBSan: 57/57 tests passed in 93.41 seconds with leak detection
  disabled because LeakSanitizer cannot run under the desktop runner's ptrace
  policy.
- MinGW Windows Release and Emscripten Web Release both compile and link.
- Native v1 and all source-backed legacy layouts are byte-identical after the
  terminal aggregate adapter round trip. The persistence journey restores and
  resaves a distinctive command, cursor, and scroll offset across restart.
- Canonical renderer, surface generation, live environment/content, scripted
  journey, and orbit/surface hashes and counters remain unchanged.

## Next step

Prepare the post-M17 release: select the next semantic version, update release
notes and package copy, build and verify Linux/Windows/Web artifacts, refresh
the hosted browser bundle and website download links, then publish only from a
clean tagged commit.
