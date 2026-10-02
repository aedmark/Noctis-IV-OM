# M6-W01 — mandatory Noctis IV Plus bug fixes

M6-W01 ports the bug-fix outcomes named by the pinned Noctis IV Plus release
notes. The implementation follows the final NIV+ behavior while fitting the
native port's tested boundaries; it does not revive DOS file handling.

| Ledger item | Result | Acceptance evidence |
| --- | --- | --- |
| P02 panorama safety | `DONE` | Panorama capture uses `WIDE9997.BMP`–`WIDE9999.BMP`, outside the player's `SNAP` namespace. The production composer closes all inputs, writes a sibling temporary, and publishes one complete 916×200 indexed BMP. `panorama_regression` checks the inherited seams, consecutive captures, cleanup, and preservation of a sentinel `SNAP9998.BMP`. |
| P12 `m` while labeling | `DONE` | The normal snapshot alias is accepted only outside star/planet label entry; the original `*` binding remains available. `ship_interface` pins both cases at the semantic input boundary. |
| P15 planet note count | `DONE` in M5-W05/W06 | Native GOESnet looks up the selected planet's object ID. `goesnet_application` verifies FELYSIA's planet-specific count through the production application workflow; see `GOESNET_WORKFLOWS.md`. |
| P16 Epoc 6012 triad | `DONE` | Both cockpit and outer-HUD displays use one modulo-billion splitter. `ship_interface` verifies `999.999.999` immediately before the boundary and `000.000.000` at Epoc 6012. |
| P17 three-digit triads | `DONE` | All three components use a shared three-digit format. The focused fixture verifies `007.008.009`. |
| P18 historic Windows startup outcome | `DONE` in M7-W01 | Hosted MSVC builds and 34 platform-independent tests pass. The portable Release package runs on native Windows; its exact executable also passes diagnostics and the bounded three-frame graphical smoke under Wine. See `WINDOWS_MSVC.md`. |
| P21 `s`/`p` while labeling | `DONE` | Label entry owns printable keys before cockpit action dispatch, and the dispatcher also rejects all ship actions while a label is active. The fixture explicitly submits `s` and `p`; M6-W02 must retain this guard when adding roof speed. |

The panorama output-number limit is not widened here. Eight-digit NICE-style
numbering is P05 and belongs to M6-W02. Likewise, this work establishes the
label-safe routing contract for `s`; the roof-speed feature itself is P03.

## Source-history anchors

- Final NIV+ `source/NOCTIS-0.CPP` writes forced panorama frames as `WIDE%04d`.
- Final NIV+ `source/NOCTIS-1.CPP` composes those frames and closes them before
  removal. The LR baseline instead uses `SNAP9997`–`SNAP9999`.
- NIV+ commit `22e0d8c` guards the `s` and `p` actions while labeling.
- Release 2.2 (`dd756a2`) and the final source use modulo one billion for the
  sinister triad at the Epoc 6012 boundary.

## Linux validation

The acceptance target is the full Clang, GCC, and Clang ASan/UBSan CTest
matrix. Windows validation remains separately assigned by ADR-0009.
