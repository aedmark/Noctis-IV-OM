# M17-W05 deterministic input recording and replay

Status: COMPLETE

## Boundary

Recordings contain semantic `InputFrame` values after platform polling and
control mapping, paired with authoritative simulation tick numbers. They do
not contain Raylib key codes, host timestamps, rendered frames, or serialized
engine state. This keeps one recording portable across desktop and browser
input backends and makes presentation cadence irrelevant to input delivery.

`InputReplay` returns the frame recorded for a requested tick and a neutral
frame for an unrecorded tick. Tick requests must advance strictly. Skipping a
recorded tick or requesting a prior tick latches `missed_input`; acceptance
fixtures require both `complete()` and `!missed_input()`.

## Version 1 wire format

All integers and IEEE-754 float bit patterns are little-endian. Boolean fields
are assigned stable flag bits rather than copied from compiler-dependent
structure storage.

| Field | Size | Rule |
| --- | ---: | --- |
| Magic | 4 bytes | ASCII `NIRP` |
| Version | `u16` | `1` |
| Reserved | `u16` | zero |
| Frame count | `u32` | at most 1,000,000 |
| Tick | `u64` | strictly increasing |
| Input flags | `u64` | bits 0–39 are defined; all others zero |
| Mouse delta X/Y | two `u32` | finite float bit patterns |
| Text count | `u16` | at most 64 codepoints |
| Frame reserved | `u16` | zero |
| Text | repeated `i32` | semantic Unicode codepoints |

The decoder rejects bad magic, unsupported versions, nonzero reserved fields,
unknown flags, excessive counts, duplicate or decreasing ticks, non-finite
mouse deltas, truncation, trailing bytes, and oversized files. Failed decode
does not mutate the caller's existing recording. File publication uses the
same-directory atomic replacement boundary used by native saves.

## Acceptance evidence

`input_recording` covers all 40 boolean fields, exact float bits, Unicode text,
byte-stable round trips, malformed inputs, sparse replay, missed-tick
detection, and atomic file save/load.

`scripted_journey` now serializes and decodes its five input events before
running. The decoded recording reaches the same four exact state and indexed
framebuffer checkpoints when presentation runs every tick and every fourth
tick.

`orbit_surface_journey` records all 783 semantic input frames from the live
blocking surface session. A second process starts from an independently seeded
profile and replays that file while presenting every fourth rendered frame.
Both runs reach touchdown at frame 488, return at frame 538, restore the exact
ship position, and retain identical orbit, ground, outbound, capsule, and
content counters. The normal-cadence run presents 817 frames; the replay run
presents 205.

The fixture-only CLI switches are `--record-input PATH`, `--replay-input PATH`,
and `--fixture-presentation-interval N`. They deliberately require
`--orbit-surface-fixture`; a player-facing recorder is outside W05.
