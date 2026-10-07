# Session Handoff

Replace this document at the end of every session; Git holds older versions.

## Session identity

- Date: 2026-10-06.
- Repository: local `Noctis-IV-OM`; branch `master`.
- Active milestone: post-M17 browser Moviemaker maintenance.
- Published checkpoint: v3.2.0 release; M17 remains complete.
- Working tree: clean after the v3.2.0 release commit.

## Read first

1. `RELEASE_3_2_0.md` for package hashes and release verification.
2. `NIVPLUS_MOVIEMAKER.md` for capture, playback, and export behavior.
3. `ENGINE_CONSOLIDATION.md` for M17 rules and preserved invariants.
4. M17 in `ROADMAP.md` and ADR-0015 in `DECISIONS.md` for architecture context.

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

## Browser Moviemaker maintenance complete

- Completed browser decks export directly to WebM from the projector, F3, and
  GOESnet paths instead of instructing the player to begin another recording.
- The browser encoder reads clean BMP deck frames into an offscreen canvas, so
  viewport letterboxing and capture indicators cannot enter the downloaded file.
- The native capture indicator scales at 1x, 2x, and 4x internal resolutions.
- The F3 panel displays playback, stepping, looping, close, and export controls.
- Ctrl-plus/minus and numpad variants select decks in the active web game without
  changing the browser zoom level.

## Verification

- Linux Clang Release: 57/57 tests passed in 7.46 seconds.
- Linux GCC Debug: 57/57 tests passed in 27.84 seconds.
- Clang ASan/UBSan: 57/57 tests passed in 94.28 seconds with leak detection
  disabled because LeakSanitizer cannot run under the desktop runner's ptrace
  policy.
- MinGW Windows Release and Emscripten Web Release both compile and link.
- Native v1 and all source-backed legacy layouts are byte-identical after the
  terminal aggregate adapter round trip. The persistence journey restores and
  resaves a distinctive command, cursor, and scroll offset across restart.
- Canonical renderer, surface generation, live environment/content, scripted
  journey, and orbit/surface hashes and counters remain unchanged.
- Extracted Linux package diagnostics and three-frame graphical smoke passed.
- Extracted Windows package diagnostics passed under Wine.
- Linux, Windows, and Web archives match their published SHA-256 files.
- A browser-exported deck decoded as a playable 640x400 VP9 WebM without the
  reported white viewport rectangle.
- The release website passed desktop and mobile responsive visual checks; the
  WebAssembly payload reached its launch screen without console errors.

## Next step

Plan the next engine milestone. The next safe persistent-state candidate remains
the compact FCS status aggregate, but it should move only with its HUD mutation
paths and the same native/legacy adapter evidence used by M17-W07.
