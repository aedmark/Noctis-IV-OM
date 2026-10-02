>>>
Please Note: This changelog only includes actual changes to the mechanics of the
game, not translations of functions from Assembly to C++.
>>>

# Versions

## 1.0.0 (2026-10-01) — General Availability

Noctis IV OM 1.0.0 is the first stable, general availability release of the
native C++20 modern port of Alessandro Ghignola's space exploration game
*Noctis IV*, combining the native C++ foundation of Noctis IV LR with the
features, identity, and content of Noctis IV Plus.

### Key Highlights
* **Native C++20 Core:** Runs natively on 64-bit Linux and Windows without DOSBox.
* **Deterministic Galaxy:** Preserves original procedural generation across all star classes, planets, moons, surface features, and catalogs.
* **Noctis IV Plus Parity:** Includes restored Moviemaker (F3), extended cockpit telemetry, seamless window borders, and high-precision timing.
* **Portable by Default:** All player saves, catalogs, screenshots, and movies stay contained within the game directory, with full support for OS-native directories (`--system-user-data`) or custom profiles (`--user-data-dir`).
* **Non-Destructive Migration:** Safe legacy profile import (`--prepare-user-data --migrate-from`) never deletes or overwrites existing player data.

### Controls & Navigation
* Modern WASD navigation and mouse-look camera controls (toggle with `Tab`).
* Cockpit terminal interaction via right-click screen focus.
* Seamless F1 contextual help and F2 visual preference overlays.

### Stability & Quality Hardening
* Resolved ship lithium depletion on game relaunch.
* Hardened software rasterizer against buffer overruns in 2D flare line drawing.
* Corrected polygon additive flare blending operator precedence.
* Fixed font glyph underflow in digit rendering, restoring row-0 characters.
* Replaced non-portable `gcvt` with standard `std::snprintf`.
* Added bounds checks across pixel blitting routines.
* Fully validated with 40/40 passing unit and fixture tests across Clang, GCC, and AddressSanitizer/UBSan with zero leaks.
