# M5-W03 Ship Interface

Status: complete on Linux. Windows verification remains deferred by ADR-0009.

## Scope boundary

This work item closes the inherited LR ship interface: the three physical wall
screens, the onboard manager and its command pages, the flight HUD, the native
keyboard bridge, and the four LR preferences. It does not pull the Noctis IV
Plus additions forward from M6. F1 help, F2 visual settings, mouselook, roof
speed, visor/HUD visibility, and lens-flare presentation remain in M6-W02/W03;
their saved bytes are merely preserved by M5-W01/W02.

## Reachable interface inventory

The central onboard manager has four top-level pages selected with `5`, `R`,
`P`, and `X`; `6`–`9` activate the four visible rows.

| Page | Routes |
| --- | --- |
| Flight control | Remote target, Vimana flight, local target/fine approach, surface capsule |
| Onboard devices | Navigation, miscellaneous, galactic cartography, emergency functions; each has four commands |
| Preferences | Automatic screen sleep, reversed pitch, persistent menus, hull polarization |
| Display | Turns the central manager display off |

The right wall has three selectable screens:

| Screen | Behavior and controls |
| --- | --- |
| 0 — GOES command | Printable command entry, Backspace editing, Enter execution, Home clear, Escape pass-through |
| 1 — GOES output | Up/Down by line, Page Up/Page Down by page, Home/End boundaries |
| 2 — planetary atlas | Arrow-key landing-point movement, accelerated movement, Enter deploy, Escape cancel |

The flight HUD has an outer layer for remote/local labels and distances,
reset/status messages, target search, and the target list. Its inner sliding
data sheet has three routes: remote target data, local target data, and external
environment data. Selecting a wall screen deliberately suppresses the outer
label/distance layer while retaining the flight-control status line.

The remaining direct cabin controls are WASD movement, mouse look/select,
arrow-key pitch and target-list movement, `+`/`-` cabin illumination, Escape,
and the inherited snapshot keys. Snapshot extensions and other NIV+ bindings
remain assigned to M6.

## Repairs made during closure

The LR port's output viewer did not actually determine its file length: it used
the return value of `fseek`, so End normally jumped to byte zero. It also asked
`fread` for one 147-byte item and then treated the returned item count as a byte
count, placing a terminator at byte one. The output screen now uses a bounded,
tested byte reader, clamps all navigation to the available range, and always
keeps a terminator outside the 147 visible cells.

The Raylib adapter previously discarded every text character outside digits
and lowercase letters. That made `+` cabin lighting and normal GOES/label
punctuation unreachable. It now accepts printable ASCII, retains explicit
fallback mappings for synthetic input, and avoids double-enqueuing characters
reported by both Raylib paths.

Cockpit shortcut decoding and preference labels now live in the focused
`ship_interface` module used by production code. This keeps their behavior
testable without opening a window.

## Automated evidence

`ship_interface` checks all ten onboard shortcuts, all preference labels, GOES
Home/End/line/page clamping, and exact 147-byte page reads. The expanded input
test proves uppercase and punctuation delivery, including `+`.

`ship_interface_application` starts the production executable without a host
display. It verifies four complete top-level menus, all five device pages, the
three HUD data-panel routes, all four preference transitions, and preference
round-trip through `current.niv`. Together with the M3/M4 frame and journey
fixtures, this protects the reachable baseline cockpit without claiming the
Plus-only M6 presentation work or DOS-pixel identity.
