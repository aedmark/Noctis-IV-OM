# M6-W02 — Noctis IV Plus controls and quality of life

M6-W02 ports the final pinned NIV+ control behavior without importing its DOS
input or unsafe file handling. `plus_controls` is the production boundary for
key routing, snapshot allocation, mouselook, roof pacing, vertical surface
movement, and drive recharge.

## Player controls

| Context | Control | Behavior |
| --- | --- | --- |
| Space and surface | `m` or `*` | Normal snapshot with location/target data. `m` remains ordinary text during label entry. |
| Space and surface | `b` or Delete | Raw snapshot without the added location/target overlay. Label entry owns printable `b`; Delete uses a non-text semantic key. |
| Surface | `n` or `/` | Three-frame panoramic snapshot. |
| Surface | `v` or `.` | Raw panoramic snapshot with overlays suppressed on every frame. |
| Stardrifter | `s` | Toggle roof speed. The normal 55 ms presentation wait is bypassed only while the player is on the roof; switching it off cannot leave a future wait accumulated. |
| Space and surface | Down | Cycle movement mouse, mouselook, and inverted-Y mouselook. The shared mode persists in the native save. |
| Surface | `j` | Jump while at ground level. |
| Surface | `l` (or `f`) | Toggle suit torch / headlamp with forward spotlight beam. |
| Surface | Space | Fire the jetpack while outside the capsule; continued presses add upward thrust. |
| Surface | `c` | Release jetpack directional control. |

The final NIV+ source exposed Omega Drive by storing a negative lithium charge
and otherwise required binary editing. The native port retains that state but
provides a safe control: start with `--omega-drive` to equip the current save,
or `--standard-drive` to restore full conventional power and all 120 lithium
charges, including from a completely depleted save. Omega
recharge restores power without consuming the negative enablement value and
persists across restart.

## Snapshot sequence

Ordinary and panoramic outputs share the persisted `last_snapshot` counter and
the final NICE-style `00000000.BMP` through `99999999.BMP` namespace. Allocation
starts after the saved number, skips occupied files, wraps once at the upper
boundary, and refuses an inaccessible or completely full namespace. Panorama
working frames remain in the private `WIDE9997`–`WIDE9999` namespace established
by M6-W01.

## Acceptance evidence

| Ledger rows | Evidence |
| --- | --- |
| P01, P04, P24 | `plus_controls` pins every normal, raw, panorama, and raw-panorama key, including label and in-progress-panorama suppression. `input_mapping` verifies Delete remains non-text. |
| P03 | The focused pacing gate proves waiting is removed only for roof plus enabled roof speed; the two-process persistence journey restores the option. |
| P05 | A filesystem fixture skips an occupied eight-digit output and wraps from 99,999,999; production snapshots and panoramas both use the same allocator and saved counter. |
| P06 | Focused fixtures pin jump impulse, sustained jetpack thrust, capsule rejection, and `c` cutoff; the production surface physics owns the resulting state. |
| P07, P23 | Exact movement/look deltas are covered for space and surface, including inverted Y, and the three-mode preference survives the application restart fixture. |
| P08 | Focused normal/Omega recharge checks prove infinite fuel does not consume its flag. The application fixture enables Omega through the public option, saves, restarts without it, and verifies the state. |

F1 help, surface status presentation, target radius, extended object viewfield,
and the F2 visual-effects controls were completed in M6-W03. Moviemaker is
restored as M6-W06 under ADR-0013.
