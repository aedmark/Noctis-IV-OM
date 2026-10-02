>>>
Please Note: This changelog only includes actual changes to the mechanics of the
game, not translations of functions from Assembly to C++.
>>>

# Versions

## 1.4.0 (2026-10-02) — The Celestial & Display Update

Noctis IV OM 1.4.0 introduces modern presentation and display enhancements,
real-time dynamic planetary axial rotation, an interactive timewarp multiplier
slider HUD, and observation deck control parity.

### Modern Display & Presentation
* **Aspect Ratio Preservation (<kbd>F8</kbd>):** Authentic 4:3 CRT proportions with dynamic pillarboxing/letterboxing preserving canonical round celestial bodies on widescreen displays, toggleable to 16:10 square pixels or 16:9 stretch mode via `F8`.
* **Fullscreen & Window Management:** Seamless fullscreen toggling with <kbd>F11</kbd> or <kbd>Alt+Enter</kbd>. Dynamic window resizing with `FLAG_WINDOW_RESIZABLE` and defensive minimum 640x480 clamping.
* **High-DPI HUD Overlay:** Full native display resolution HUD overlay featuring frosted glass badges, glowing telemetry pips, and smooth fadeouts for transient messages, aspect ratio shifts, audio mute, and simulation rates.

### Dynamic Planetary Axial Rotation
* **Real-Time Axial Rotation in Space:** Fixed the legacy engine quirk where planetary bodies remained statically frozen during orbit. Planetary bodies now dynamically rotate around their spin axes per-frame based on the simulation clock and star-specific rotation rates (`nearstar_p_rotation[n]`).
* **Sun-Locked Terminator & Cache Refresh:** Smooth planetary rotation while keeping the day/night terminator locked to the sun, invalidating surface rendering cache (`npcs = -12345`) whenever axial rotation advances by 3° or more to guarantee stutter-free celestial tracking.

### Timewarp Multiplier Slider System
* **Interactive HUD Slider:** High-DPI frosted glass timewarp slider with responsive mouse dragging, step buttons `[-]` / `[+]`, and direct numerical multiplier readouts from 1x to 5000x.
* **Key Stepping & Universal Toggle:** Step simulation speed dynamically with bracket keys <kbd>[</kbd> / <kbd>]</kbd>. Toggle timewarp on/off with <kbd>T</kbd> or <kbd>Shift+S</kbd> seamlessly across the ship cabin, observation deck, and planetary surfaces.
* **Deck Input Parity:** Observation deck input handlers now support timewarp toggling (<kbd>T</kbd> / <kbd>Shift+S</kbd>) and the <kbd>F2</kbd> visual effects menu (<kbd>T</kbd>, <kbd>F</kbd>, <kbd>B</kbd>).

## 1.3.0 (2026-10-02) — The Aural Update

Noctis IV OM 1.3.0 introduces a completely procedural ambient audio engine and
exploration Foley soundscape synthesized natively in real-time with zero external
asset dependencies, dynamic Vimana warp acoustic response, canonical observation
deck frame pacing, and control stabilization.

### Procedural Audio Engine
* **Self-Contained Real-Time Synthesis:** Fully procedural, mathematical sound synthesis running at 44.1 kHz 32-bit floating point via Raylib's miniaudio backend. Zero external `.wav`, `.ogg`, or downloaded sound assets.
* **Calm Stardrifter Cabin Drone:** Meditative, non-fatiguing 55 Hz fundamental hum with gentle 27.5 Hz sub-bass warmth, quiet 110 Hz harmonic, soft CRT monitor purr, and dual-filtered pink noise life-support ventilation breathing while aboard the Stardrifter.
* **Deep Space Observation Deck Ambience:** Stepping out onto the roof through the cupola (`ontheroof`) cuts hull resonance and transitions to expansive sub-bass cosmic ambience and diffuse stereo solar wind noise.
* **Vimana Drive Acoustic Dynamics:** Dynamic acoustic response tracking physical hyperlight travel speed and phase. Fundamental frequency starts low (~28 Hz) at ignition, rises and pulses dynamically with travel velocity up to cruising warp (~96 Hz), gently decelerates during arrival, and concludes with a smooth 1.8-second arrival spool-down without abrupt audio cuts.
* **Planetary Atmosphere Wind & Weather:** 2-pole resonant State Variable Filter (SVF) dynamically tracking planetary atmospheric pressure (`pp_pressure`) and atmospheric existence (`atmosphere`). Airless worlds (`atmosphere == 0`) feature absolute exterior vacuum silence with subtle interior suit life-support hum. Atmospheric worlds synthesize natural dual-LFO wind gusts, howling resonances, rain droplet hiss, and distant rolling thunder rumbles.
* **Exploration Foley:** Crisp mechanical dual-transient switch click for suit torch toggle (`L`), high-tech dual-frequency glide for visor servo actuation (`Page Up` / `Page Down`), pressurized cold-gas hiss burst and sustained burn for atmospheric jetpack (`Space`), and subtle regolith footsteps while walking.
* **Audio Controls & Headless Fallback:** Global audio mute toggle with `F9` or `Ctrl+M` displaying native HUD status (`AUDIO MUTED` / `AUDIO ACTIVE`), `--no-audio` CLI flag, and safe headless fallback in CI or soundless environments.

### Engine Stabilization & Controls
* **Observation Deck Frame Pacing:** Eliminated legacy DOS `ROOFSPEED` frame-limiter bypass in `swapBuffers()`. Canonical 18.2 FPS (55 ms per tick) simulation pacing is now strictly maintained on the observation deck, preventing runaway multi-thousand FPS acceleration and uncontrollable movement on modern hardware.
* **WASD Backward Movement (<kbd>S</kbd>):** Removed legacy raw `'s'` key intercept that previously triggered `ROOFSPEED` toggles and aborted the main loop frame, allowing <kbd>S</kbd> to function purely and cleanly as WASD backward walk.
* **Backwards Compatibility:** Preserved `roof_speed` serialization in `NativeSaveState` and existing save fixtures to maintain 100% round-trip compatibility with previous saves.

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
