# M17 engine consolidation

Status: COMPLETE

## Purpose

Noctis IV OM reached feature completeness while retaining the inherited global
runtime at its center. M17 introduces explicit ownership and test seams around
that runtime without changing deterministic generation, simulation results,
save schemas, or indexed rendering behavior.

This is an incremental strangler migration, not a rewrite. Each extraction must
leave the game playable and the complete compatibility suite enforceable.

## State ownership rule

`noctis::EngineState` is the root for mutable application and simulation state
extracted from the legacy translation units. New engine state must be placed in
that root or in an owned subsystem; new cross-file `extern` state is prohibited.

The first slice owns transient travel presentation state: the current travel
phase and normalized speed consumed by audio telemetry. `GoesTerminalState`
now owns the first persistent aggregate: command buffer, command cursor, and
output scroll offset. Compatibility-sensitive position, guidance, and
targeting values remain in the legacy state until they can migrate together
with save capture, restore, and journey fixtures. There must never be two
authoritative copies.

## Save-backed state rule

Runtime aggregation does not imply an on-disk schema change. Explicit capture
and restore adapters map `GoesTerminalState` to the existing flat
`NativeSaveState` fields, so native v1 remains exactly 401 bytes and every
accepted legacy layout normalizes identically. All live terminal mutations use
the engine-owned aggregate; the former command, cursor, and scroll globals no
longer exist. See `SAVE_STATE_AGGREGATES.md`.

## Platform boundary rule

Code that performs a player action may receive platform-selected paths and
services, but it must not discover host state when a caller can provide it. The
GOESnet image-export path now follows this rule:

1. Desktop composition resolves the Downloads directory.
2. GOESnet receives that directory in `GoesCommandContext`.
3. Tests inject a directory inside their fixture workspace.
4. Web leaves the destination absent and uses its browser download bridge.

Later slices will apply this pattern to clocks, presentation, audio, and
external process launching.

## Application mode rule

`ApplicationRuntimeState` records the active top-level mode and accepts only
legal transitions:

- cockpit to descent/ascent transfer, gallery, movie player, or shutdown;
- descent to surface or cockpit;
- surface to ascent transfer or cockpit;
- gallery and movie player back to cockpit; and
- any live mode to shutdown, which is terminal.

Rejected transitions do not change the mode or transition counter. The current
blocking surface loop is wrapped in an RAII session boundary, so fixture exits,
load failures, surface aborts, and ordinary capsule return all restore a valid
mode. This characterizes current ownership without yet replacing loop control.

## Deterministic input rule

The recording boundary is the semantic `InputFrame`, after platform polling
and mapping but before the legacy compatibility adapter. Records use explicit
simulation ticks and a versioned little-endian format; they never encode host
key codes, wall-clock timing, or structure padding. Replay treats skipped or
out-of-order input as an error and remains independent of presentation cadence.
The format and validation contract are specified in `INPUT_RECORDING.md`.

## Renderer memory rule

Renderer access carries its real capacity. The active indexed framebuffer is
bounded by current width × height, and texture sources bind pointer and byte
capacity together. Legacy 16-bit texture addressing remains intact inside that
view; out-of-range reads resolve to indexed color zero and out-of-range writes
are discarded. The 200×200 surface map and maximum 1280×800 framebuffer now
use exact allocations rather than compatibility padding. See
`RENDERER_BOUNDS.md`.

## Migration sequence

1. Establish `EngineState` and migrate transient travel state.
2. Inject filesystem/export destinations and remove host-directory assumptions
   from tests.
3. Model the top-level cockpit, descent, surface, gallery, and movie modes as
   explicit application transitions.
4. Separate simulation stepping from presentation and add deterministic input
   recording/replay for a complete journey.
5. Put bounded interfaces around framebuffer and surface-map writes, then
   remove compatibility padding where fixtures prove it safe.
6. Continue migrating coherent state aggregates, with save-schema adapters at
   the boundary.

## Invariants

- Galaxy, system, surface, and renderer fixture outputs do not change.
- The fixed 55 ms simulation tick remains authoritative.
- Native save version 1 and legacy imports remain byte compatible.
- Browser, Linux, and Windows continue sharing simulation code.
- No work item is complete without focused tests and the full native lane.
- Refactoring does not silently alter a legacy quirk; intentional changes need
  a decision record and new acceptance evidence.

M17 closed on 2026-10-06 after all work items and exit criteria passed. The
criterion-by-criterion evidence is recorded in `M17_EXIT_AUDIT.md`.

## First-slice evidence

- `tests/engine_state_test.cpp` protects state defaults, transitions, clamping,
  and reset behavior.
- `tests/goesnet_commands_test.cpp` injects and verifies its image-export
  destination instead of writing to the developer's Downloads directory.
- `tests/engine_state_test.cpp` also verifies the legal application-mode graph,
  rejected transitions, terminal shutdown, and reset behavior.
- The landing-return fixture asserts the production cockpit → descent → surface
  → ascent → cockpit sequence and exact transition count.
- Movie-player tests verify that failed opens retain cockpit ownership and that
  successful open/close restores it.
- Input codec tests cover every semantic field, malformed recordings, atomic
  publication, sparse playback, and missed-tick detection.
- The scripted space journey replays one decoded recording at 1× and 4×
  presentation cadence with identical exact checkpoints.
- The complete orbit-to-surface fixture records 783 blocking-loop frames and
  replays them from a separately seeded profile at 4× presentation cadence,
  preserving all simulation, indexed-frame, and content checkpoints.
- Renderer fixtures protect the active framebuffer boundary with a sentinel
  and exercise explicit one-byte texture capacity; the complete sanitizer lane
  runs with exact framebuffer and 200×200 surface-map allocations.
- Representative surface fixtures cover the repaired FELYSIA crevasse border
  writes while retaining every accepted height, texture, object, and live-frame
  hash.
- Engine-state and save tests exercise terminal defaults, reset, capture, and
  restore. Native v1 and each source-backed legacy layout remain byte-identical
  after an aggregate adapter round trip.
- The persistence journey carries a distinctive terminal command, cursor, and
  scroll offset across a process restart and confirms the same fields resave.
- Existing travel, scripted-journey, orbit/surface, audio, save, and renderer
  fixtures remain the behavioral regression boundary.
