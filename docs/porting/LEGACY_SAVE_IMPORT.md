# Legacy Save Import

M5-W02 replaces the active raw-read paths for `current.bin` and `surface.bin`
with exact-size, bounded importers. A legacy file is decoded into temporary
fixed-width state, checked, and only then applied. Successful imports write the
corresponding native v1 file without modifying the original.

## Situation layouts

The accepted sizes come from the checked-in vanilla/NIV+ source history and
the LR-derived writer. Sizes not in this table are rejected in full; no partial
read is applied.

| Bytes | Layout | Imported outcome |
| ---: | --- | --- |
| 245 | Documented original core | Core ship, target, travel, and epoch state; later fields take native defaults |
| 370 | Vanilla Release 1 / LR | Core plus GOESnet cursor, output position, and 120-byte command buffer |
| 377 | NIV+ Release 2/2.3 | Adds 32-bit snapshot counter, mouselook mode, and 16-bit roof-speed flag |
| 378 | Tracked NIV+ fixtures / one preference byte | Preserves the final byte in the native HUD/visor slot |
| 379–381 | Post-2.3 preference evolution | Progressively adds HUD text, lens-flare mode, and seamless-border state; 381 is pinned `5c46de9` |
| 382 | Transitional post-2.3 HUD build | Preserves the final HUD-open flag; obsolete animation counters are normalized away |

The 378-byte history is inherently ambiguous by size: an earlier source point
used its last byte for a Stardrifter antialias experiment, while the tracked
fixtures and later source use the slot for HUD/visor state. There is no marker
that can distinguish those writers. The importer follows the pinned/tracked
meaning and documents that older experimental preference as unavailable; the
245-byte gameplay state and other extensions are unchanged.

Before application, the importer requires finite floating-point values,
bounded subsystem/target/class selectors, terminated FCS and GOESnet strings,
and valid ranges for known preferences. File loading rejects anything larger
than the largest supported layout before allocating its contents.

When `current.niv` is absent and an accepted `current.bin` is present, startup
writes a 401-byte native v1 file and reports the detected layout. A present but
invalid native file remains authoritative and fails safely; it never falls
back to legacy state.

## Surface layouts

Two source-backed emergency checkpoint layouts are accepted:

| Bytes | Layout | Imported outcome |
| ---: | --- | --- |
| 40 | DOS / pinned NIV+ | Landing coordinates, atlas position, explorer position, and view angles; HUD fields take native defaults |
| 45 | LR-derived modern baseline | The 40-byte state plus HUD animation and closure state |

Both migrate to the 65-byte `surface.niv` v1 envelope. New emergency
checkpoints are written only in the native format. Normal capsule recovery and
clean restoration remove both native and legacy surface checkpoints.

## Evidence and limits

`legacy_save_import` exercises every accepted layout, the tracked 378-byte
NIV+ fixture, extension preservation, transitional normalization, invalid
sizes and selectors, both surface layouts, and pre-read oversized-file refusal.
`native_save_application` proves real startup migration with the tracked
situation fixture and a 40-byte surface checkpoint, native-only restart, and
safe refusal of damaged native files.

M5-W07 completes the byte-by-byte native corruption matrix, every supported
legacy-to-v1 upgrade chain, interrupted-write cases, and a two-process gameplay
save/reload journey. `PERSISTENCE_HARDENING.md` records that closure. M5-W02's
bounded importer still does not claim that unknown historical layouts are
recoverable.
