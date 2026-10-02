>>>
Please Note: This changelog only includes actual changes to the mechanics of the
game, not translations of functions from Assembly to C++.
>>>

# Versions

## 1.0.1 (2026-10-02) — Patch Release

Noctis IV OM 1.0.1 resolves low-gravity landing stalls, dim-star surface
illumination, surface session persistence, and restores missing suit helmet visor
and GOES navigation key controls.

### Planetary Landing & Surface Fixes
* **Low-Gravity Descent Floor:** Enforced a minimum vertical descent acceleration floor in `planetary_main()` to prevent landing sequence soft-locks on low-gravity worlds (such as star Oakenshield's P01).
* **Descent Abort:** Allowed pressing `Escape` during descent to cleanly cancel landing and return safely to the ship cockpit.
* **Dim-Star Surface Illumination:** Added minimum atmospheric starlight and terrain ambient illumination floors for dim-star systems (`dfs <= 0.2`), preventing pitch-black / night-vision rendering artifacts.
* **Surface Power Loss & Target Guard:** Guarded orbital target state (`ip_targetted`) against power-loss reset while the player is on a planetary surface or outside the ship (`surface_active`).
* **Session Resume Target Recovery:** Added defensive nearest-body recovery for `ip_targetted` when resuming saved surface checkpoints.
* **Regression Fixtures:** Added dedicated `oakenshield_landing_fixture` test covering low-gravity descent, dim-star palette generation, surface checkpoint saving, and descent abort.

### Controls & Navigation
* **Helmet Visor Control:** Restored `Page Up` (raise helmet visor / open suit HUD) and `Page Down` (lower helmet visor / close suit HUD) on planetary surfaces.
* **GOES Paging:** Restored `Page Up` and `Page Down` for full-page scrolling in the GOES Guide reader.
* **Home & End Keys:** Mapped `Home` and `End` for clearing the GOES command prompt and jumping to the beginning/end of GOES output.

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
