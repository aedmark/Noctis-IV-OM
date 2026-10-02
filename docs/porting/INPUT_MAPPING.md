# M3 input mapping

Status: M3-W02 complete on Linux.

## Architecture

All active Raylib keyboard and mouse reads are confined to `input.cpp`.
The platform poller produces a semantic `InputFrame`; a compatibility adapter
then updates the inherited gameplay inputs:

- held movement directions (`key_move_dir`);
- scaled mouse delta and accumulated coordinates;
- the legacy two-bit mouse-button mask (`mpul`); and
- the legacy key-code stack consumed by `is_key()`/`get_key()`.

An `InputProvider` can replace the Raylib poller. Tests exercise both direct
frame mapping and provider injection without opening a window. M3-W06 uses the
same provider boundary for its tick-indexed scripted journey.

## Default physical mapping

| Physical input | Semantic field | Legacy output |
| --- | --- | --- |
| W / S | Move forward / backward | Held direction state |
| A / D | Strafe left / right | Held direction state |
| Mouse motion | Look delta | Each axis divided by 5, truncated to `int16_t`, then accumulated |
| Left / right mouse held | Primary / secondary button | `mpul` bits 0 / 1 |
| Escape held | Exit/cancel | Key code 27 each sampled frame |
| Printable ASCII text | Text/legacy command | Character code |
| Arrow keys | Navigation | DOS extended sequence: 0 then scan code 72/80/75/77 when popped |
| Backspace / Enter | Editing/confirm | 8 / 13 |
| Delete | Snapshot command | `*` |
| F1 | Toggle cursor capture | Existing Raylib cursor toggle behavior |

The LIFO key-code behavior is inherited compatibility semantics and deliberately
preserved. M5-W03 widened the native adapter from alphanumerics to the printable
ASCII range because the GOES prompt, labels, parsis coordinates, and cabin-light
controls consume punctuation too. Semantic fields for Apostrophe, Space, Plus,
Minus, Comma, Slash, and Semicolon remain as synthetic-input fallbacks; the live
provider normally supplies their printable character. Scripted inputs should
normally enqueue
at most one text character per simulation tick unless they intentionally test
that ordering.

## Scope limits

- This work establishes the injectable action/state boundary; it does not
  redesign bindings or add a user-facing remapping screen.
- Double-click recognition remains in the gameplay loop but already uses the
  monotonic presentation/input clock established by M3-W01.
- Controller, touch, and accessibility mappings are not current M3 scope.
- Windows input behavior remains deferred with the rest of Windows verification.
