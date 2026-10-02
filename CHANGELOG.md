>>>
Please Note: This changelog only includes actual changes to the mechanics of the
game, not translations of functions from Assembly to C++.
>>>

# Versions

## 1.2.0 (2026-10-02) — The Celestial Resonance Update

Noctis IV OM 1.2.0 introduces a completely procedural ambient audio engine and
exploration Foley soundscape, synthesized natively in real-time with zero external
asset dependencies.

### Procedural Audio Engine
* **Self-Contained Real-Time Synthesis:** Fully procedural, mathematical sound synthesis running at 44.1 kHz 32-bit floating point via Raylib's miniaudio backend. Zero external `.wav` or `.ogg` files or downloaded assets.
* **Stardrifter Cabin Drone:** Deep, meditative 55 Hz fundamental hum with warm 110 Hz and 165 Hz harmonics, 27.5 Hz sub-bass pulse, quiet CRT monitor purr, and gentle LFO ventilation breathing while aboard the Stardrifter.
* **Deep Space Observation Deck:** Stepping out onto the roof through the cupola (`ontheroof`) cuts hull resonance and transitions to expansive sub-bass cosmic ambience and diffuse stereo solar wind noise.
* **Vimana Drive Acoustic Dynamics:** Dynamic pitch, harmonics, and volume scaling with propulsion phase, drive speed, and orbital approach progress; energetic warp whine during charging/warm-up and steady harmonic resonance during hyperlight cruise.
* **Planetary Atmosphere Wind & Weather:** 2-pole resonant State Variable Filter (SVF) dynamically tracking planetary atmospheric pressure (`pp_pressure`) and atmospheric existence (`atmosphere`). Airless worlds (`atmosphere == 0`) feature absolute exterior vacuum silence with subtle interior suit life-support hum. Atmospheric worlds synthesize natural dual-LFO wind gusts, howling resonances, rain droplet hiss, and distant rolling thunder rumbles.
* **Exploration Foley:** Crisp mechanical dual-transient switch click for suit torch toggle (`L`), high-tech dual-frequency glide for visor servo actuation (`Page Up` / `Page Down`), pressurized cold-gas hiss burst and sustained burn for atmospheric jetpack (`Space`), and subtle regolith footsteps while walking.
* **Audio Controls & Headless Fallback:** Global audio mute toggle with `F9` or `Ctrl+M` displaying native HUD status (`AUDIO MUTED` / `AUDIO ACTIVE`), `--no-audio` CLI flag, and safe headless fallback in CI or soundless environments.

## 1.1.0 (2026-10-02) — The Torchlight Update

Noctis IV OM 1.1.0 introduces the planetary Suit Torch headlamp, high-resolution
crisp HUD status text, and critical surface session resume stabilization.

### New Features & Visual Improvements
* **Suit Torch / Headlamp (`L` Key):** Added a player headlamp toggled with `L` while exploring planetary surfaces. Illuminates nightside worlds, tidally locked dark hemispheres, and unlit caves with a focused directional beam, realistic quadratic distance attenuation, and smooth 32-bit RGBA surface shading.
* **Crisp HUD Status Overlay:** Redesigned transient HUD status messages (such as `TORCH ON`, `TORCH OFF`, `MOUSELOOK ENABLED`) to render at native display resolution directly above the retro framebuffer, eliminating text blurriness and visual degradation.

### Surface Persistence & Engine Stability
* **Surface Save Situation Synchronization:** Ensured spaceship orbital and flight state (`current.niv`) is automatically preserved with `freeze()` whenever the player quits from a planetary surface via `Escape`, preventing desynchronization between ship and surface checkpoints.
* **Stellar Reconstruction on Cold Resume:** Resolved a startup delay issue where celestial bodies were not regenerated upon cold relaunch when resuming a surface save.
* **Defensive Orbital & HUD Bounds:** Guarded celestial coordinate calculations (`planet_xyz`, `moonorigin`, `rtp`) against division by zero and `NaN` propagation on systems without bodies; hardened `wrouthud` to prevent buffer overflow when handling non-standard character codes; and safely discarded orphaned surface checkpoints without process termination.

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
