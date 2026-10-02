# Native Save Format

M5-W01 replaces the runtime's raw memory-layout persistence boundary with an
explicit, versioned native situation file. The application writes
`data/current.niv` and prefers it at startup. Since M5-W05, normal saves emit no
`data/current.bin`; that file is accepted only as a bounded legacy import source
when native state is absent.

## Version 1 envelope

All integers and IEEE-754 bit patterns use little-endian byte order. No C++
structure is copied directly to disk.

| Offset | Size | Meaning |
| ---: | ---: | --- |
| 0 | 8 | Magic `NIVSAVE\0` |
| 8 | 2 | Schema version (`1`) |
| 10 | 2 | Header size (`20`) |
| 12 | 4 | Payload size (`381`) |
| 16 | 4 | CRC-32 of the payload |
| 20 | 381 | Version 1 payload |

The payload preserves the 370-byte LR situation prefix plus all eleven
source-backed NIV+ extension bytes: ship systems and power, target-selection state, cockpit position
and orientation, current and target coordinates, travel-controller values,
FCS status, simulation epoch, panel state, the GOESnet command/output cursor,
snapshot numbering, mouselook, roof speed, visor/HUD state, and visual-effect
preferences. Each field is encoded individually with a fixed width in the same
logical order documented beside the inherited globals in `noctis-0.cpp`.

The writer creates `current.niv.tmp`, flushes and closes it, and then replaces
`current.niv`. The reader caps input at 1 MiB before allocation and accepts a
v1 file only when its magic, version, declared sizes, total size, and checksum
all agree. A present but invalid native save is an error; the application does
not silently fall back to an older `current.bin`.

## Compatibility and upgrade policy

- Missing `current.niv` invokes M5-W02's bounded importer. Accepted legacy
  layouts are immediately migrated; see `LEGACY_SAVE_IMPORT.md`.
- Unknown native versions are reported as unsupported, not interpreted as v1.
- A future schema change must increment the version and add an explicit
  conversion path; it must not change the meaning or size of v1 fields.
- Native emergency surface resumes use a parallel 65-byte `surface.niv` v1
  envelope with magic `NIVSURF\0` and a 45-byte payload. M5-W02 imports the
  source-backed 40- and 45-byte `surface.bin` layouts.
- M5-W05 retired the `current.bin` writer path with executable GOESnet modules.
  Successful legacy imports preserve their source but all new writes use
  `current.niv`.

## Evidence

`native_save_round_trip` pins the 401-byte situation and 65-byte surface
envelopes and verifies bit-exact
memory/file round trips, version refusal, size refusal, and checksum refusal.
`native_save_application` starts from the tracked legacy FELYSIA fixture,
writes native v1 through the real application adapter, removes the legacy
input, and starts successfully from native state alone. It also proves that a
damaged native file stops restoration with a structured diagnostic.

Together with `legacy_save_import` and M5-W07's `persistence_journey`, these
tests establish the v1 boundary, bounded migration, application integration,
every-byte corruption refusal, semantic validation after checksum verification,
failed-replacement durability, and continued play across a process restart.
See `PERSISTENCE_HARDENING.md`.
