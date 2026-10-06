# M17-W07 save-backed state aggregates

Status: COMPLETE

## First migrated aggregate

`GoesTerminalState` is owned by the application-wide `EngineState` and contains
the three values that form a persisted terminal session:

- the 120-byte GOESnet command buffer;
- the command cursor; and
- the output scroll offset.

The former independent `goesnet_command`, `gnc_pos`, and `goesfile_pos`
globals have been removed. Keyboard editing, command dispatch, scrolling,
bookmark routing, fixture command injection, save capture, and load restore all
use the engine-owned state. There is no compatibility alias or second live
copy.

## Persistence boundary

Native save version 1 deliberately remains flat. The two explicit adapters
`capture_goes_terminal_state()` and `restore_goes_terminal_state()` translate
between runtime ownership and the existing `NativeSaveState` fields. The
codec's field order, widths, payload size, checksum, validation, and version are
unchanged:

| Boundary | Preserved contract |
| --- | --- |
| Native situation | 20-byte envelope + 381-byte v1 payload = 401 bytes |
| Legacy situation | Accepted 245, 370, 377, 378, 379, 380, 381, and transitional 382-byte layouts |
| Terminal command | 120 bytes at the same payload position |
| Cursor and scroll | Signed 8-bit cursor and signed 32-bit scroll offset at the same positions |

This separation lets runtime state continue consolidating without coupling
engine ownership to disk layout or requiring a gratuitous schema version.

## Acceptance evidence

`engine_state` verifies defaults, restore, capture, and reset for the complete
aggregate. `native_save_round_trip` decodes a distinctive v1 state, restores it
into the aggregate, captures it back into a clean boundary object, and proves
the full 401-byte encoding is unchanged.

`legacy_save_import` performs the same adapter round trip for every
source-backed legacy situation size before migration and proves the normalized
native encoding is byte-identical. The application `persistence_journey` saves
`ST FELYSIA_`, cursor 10, and scroll offset 42, starts a new process, verifies
all three live values, resaves, and verifies the same flat save fields.

The complete Linux Clang Debug and GCC Debug suites pass 57/57. The complete
Clang ASan/UBSan suite passes 57/57 with leak detection disabled because the
desktop runner's ptrace policy prevents LeakSanitizer operation. MinGW Windows
Release and Emscripten Web Release both compile and link. Existing universe,
renderer, surface, and journey fixtures retain their exact hashes and counters.
