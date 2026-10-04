>>>
Please Note: This changelog only includes actual changes to the mechanics of the
game, not translations of functions from Assembly to C++.
>>>

# Versions

## 1.9.0 (2026-10-04) — The Audio Controls & Exploration Ergonomics Update

### Features
* **Audio Category Volume Controls:**
  * Added five independent, persistent volume categories: Master, Spaceflight & Maneuvering, Cockpit Foley, Visor / Suit, and Environment / Surface.
  * Interactive High-DPI Audio Options overlay in the graphics menu (<kbd>F2</kbd> or <kbd>Tab</kbd>/<kbd>A</kbd>) supporting tab clicking, mouse slider dragging, keyboard stepping (<kbd>+</kbd>/<kbd>-</kbd> or arrow keys), number keys (<kbd>1</kbd>–<kbd>5</kbd>), and global mute (<kbd>M</kbd>).
  * Persistent volume levels saved across game sessions to `config.ini` in the user configuration directory.
* **Cursor Unlock & Screenshot Ergonomics:**
  * Pressing <kbd>F10</kbd> to unlock the mouse now immediately freezes player and camera movement, zeroing residual rotational inertia (`dlt_alfa`, `dlt_beta`) and translation momentum so external screenshot utilities capture perfectly stable views.
  * Mouse and keyboard motion inputs are gated while the window is unfocused or cursor is unlocked, preventing accidental character movement when interacting with overlays or external apps.
  * Eliminated cursor recentering/warp jerks when restoring window focus or re-locking the cursor with <kbd>F10</kbd>.

## 1.8.0 (2026-10-04) — The Acoustics & Surface Exploration Update

### Features
* **Flight & Maneuvering Acoustics:**
  * Sublight attitude navigation steering and collision avoidance now trigger procedural cold-gas RCS thrusters, featuring an onset valve pop and continuous stereo thruster hiss.
  * Atmospheric descent now produces physical aerodynamic buffeting turbulence, driven dynamically by descent velocity and planetary atmospheric pressure through resonant low-pass filtering.
  * Planetary touchdown now features a dual-stage physical clunk combining low-frequency hull resonance, metallic landing gear latch rings, and regolith compression crunch on impact and rebound.
* **Cockpit & GOESnet Foley:**
  * Mechanical vintage solenoid/spring keyboard clacks with three round-robin procedural variations for GOESnet console typing, star catalog prompts, and celestial labeling.
  * Optical telemetry transmit burst chirps on GOESnet command submission (<kbd>Enter</kbd>).
  * Dual-harmonic bell acknowledge chimes for successful commands and valid locks vs retro dual square/sine error buzzes on command errors or out-of-range targets.
  * Tactile dashboard rocker switches and console button clicks for cockpit instrument interaction, FCS commands, and bulkhead screen selection.
  * Linefeed scroll taps for Guide browsing and landing target coordinate cursor adjustments.
  * Observation deck hydraulic elevator carriage servo glide when ascending or descending to the deck.
* **Surface Exploration & Visual Fidelity:**
  * Planetary suit torch / headlamp (<kbd>L</kbd>) overhauled with direct albedo luminance scaling, naturally illuminating terrain textures and polygon meshes without an artificial white overlay.
  * Planetary exploration footsteps now synchronize strictly to active movement inputs, preventing ghost steps during standing turns or idle frames.
  * Airless worlds now feature a soothing spacesuit life-support ventilation hum instead of vacuum silence buzzing.
  * Jetpack flight dynamics rebalanced with a 1,200-unit flight ceiling and smooth vertical velocity damping, preventing infinite space escape while eliminating uphill mid-air collision hitches ("invisible walls").

### Fixes
* **Observation Deck Camera Rotation:** Decoupled observation deck player camera look and steps from spacecraft attitude RCS thrusters, preventing cold-gas valve bursts when looking around the deck.

## 1.7.1 (2026-10-03) — Native Wayland & Controls Update

### Features
* **Native Wayland Support:** Linux builds now include native Wayland support alongside X11. The game automatically connects natively on Wayland compositors (GNOME, KDE Plasma, etc.) and seamlessly falls back to X11 on X11 sessions.

### Fixes
* **Browser Mouselook:** The web build now captures the mouse when you press LAUNCH. <kbd>Esc</kbd> releases it, clicking the game captures it again, and <kbd>F10</kbd> turns mouselook on or off. Browsers only allow mouse capture straight from a click or key press, so the page now handles it itself instead of the game.
* **Browser Fullscreen:** <kbd>F11</kbd> and <kbd>Alt</kbd>+<kbd>Enter</kbd> now toggle fullscreen in the browser. Previously F11 was swallowed and Alt+Enter's delayed request was rejected.
* **Desktop F10:** The first <kbd>F10</kbd> press after startup now releases the mouse. Previously the game thought the mouse started released, so the first press re-captured it and did nothing visible.
* **Polygon Vertical Gradients:** Corrected vertical scanline gradient calculation in 2D triangle rasterization (`case 4` in `draw_triangle_2d`), ensuring proper clamping and signed gradient stepping.
* **Codebase Cleanup:** Verified and cleaned up legacy assembly registers in star sector position hashing (`isthere`), verified `single_pixel_at_ptr` offset against vanilla, and resolved all remaining `TODO`/`FIXME` comments in `src/`.

## 1.7.0 (2026-10-02) — The Web Update

Noctis IV OM 1.7.0 takes the Stardrifter into the browser. The complete game now
builds for WebAssembly and is playable on the project website, with saves kept in
the browser, alongside the native Windows and Linux releases.

### Play in the Browser
* **Full Game on the Web:** Play at <https://aedmark.github.io/Noctis-IV-OM/play/> with no download or install: the same deterministic software renderer, cockpit, GOESnet, planetary surfaces, image archive, and procedural audio as the desktop builds.
* **Browser Saves:** Your ship, surface position, catalogs, gallery, and settings persist in the browser's IndexedDB storage. The game autosaves about every 30 seconds and whenever the tab is hidden or closed, so you can simply close the tab.
* **Browser-Friendly Controls:** <kbd>Esc</kbd> never ends a browser session, because browsers also use it to release the mouse and leave fullscreen; landed on a planet, it saves in place. Browser shortcuts that clash with game keys (F1–F4, F6–F11, arrows, Space) are held back while playing.
* **Any Window Size:** The canvas follows the browser window, from phones to ultrawide monitors, and the game letterboxes its authentic 4:3 frame. The CRT shader (<kbd>F6</kbd>) runs on WebGL2.
* **Self-Hosting:** A downloadable `Noctis-IV-OM-web.zip` bundle runs from any static web server.

### Website
* **Play in Browser:** The site gains Play in Browser links and is now deployed to GitHub Pages automatically from `master`.
* **Noctis IV Manual:** The site's manual links now open the preserved Noctis IV Plus manual instead of a missing page.

### Fixes
* **Quick Key Taps:** Keys tapped faster than a single frame (55 ms) were sometimes ignored. Input now also reads every press the frame queued, on all platforms.

## 1.6.0 (2026-10-02) — The Image Archive Update

Noctis IV OM 1.6.0 brings the player's photo album aboard the Stardrifter: an
in-cockpit image archive for browsing past snapshots and panoramas, GOES console
commands to list and open them, capture confirmations, and snapshot files that
strict image tools can now open.

### Cockpit Image Archive (<kbd>F4</kbd>)
* **In-Cockpit Viewer:** Browse every snapshot and panorama in the player gallery without leaving the game. Press <kbd>F4</kbd> aboard the Stardrifter to open the newest image; <kbd>Left</kbd>/<kbd>Right</kbd> (or <kbd>Page Up</kbd>/<kbd>Page Down</kbd>) browse, <kbd>Home</kbd>/<kbd>End</kbd> jump to the oldest/newest, and <kbd>Esc</kbd>, <kbd>Enter</kbd>, or <kbd>F4</kbd> closes.
* **GOES Console Commands:** `GALLERY` lists the archive newest-first on the GOES output screen; `VIEW` opens the newest image and `VIEW n` opens a specific number (`VIEW 42`, `VIEW 00000042`, or legacy `VIEW SNAP0003`).
* **Panorama Panning:** <kbd>Z</kbd> or <kbd>Space</kbd> zooms an image to the full viewer height; while zoomed, <kbd>Left</kbd>/<kbd>Right</kbd> pan across 916-pixel panoramas with a position indicator.
* **Capture Confirmation:** Snapshots and panoramas now show a brief `SNAPSHOT 00000042 SAVED` notice (or a failure message). It is drawn only in the high-resolution overlay, so it never appears in the next snapshot or in Moviemaker frames.
* **Faithful Colors:** Each image is decoded with the palette saved inside its own BMP and shown at the same pixel aspect as the live view, drawn at full window resolution above the retro canvas so the game palette is never disturbed.

### Fixes
* **Standards-Compliant Snapshot BMPs:** Ordinary 320x200 snapshots previously inherited stale file-size and image-size fields from the archived DOS header template, so strict decoders such as ImageMagick rejected them. Snapshots and panoramas now share one header writer that fills in the correct sizes.

## 1.5.0 (2026-10-02) — The Visual Fidelity & Post-Processing Update

Noctis IV OM 1.5.0 introduces modern post-processing and visual fidelity enhancements,
featuring deterministic edge-directed upscaling, an authentic OpenGL GLSL CRT monitor
simulation shader pipeline, sub-pixel geometry rasterization, antialiased celestial
bodies, and persistent user configuration.

### Deterministic Edge-Directed Upscaling (<kbd>F7</kbd>)
* **Scale2x / EPX Filtering:** Deterministic edge-directed upscaling expanding the retro 320x200 canvas to 640x400. Smooths jagged pixel staircasing along diagonal boundaries while strictly preserving the original 256-color palette and color purity.
* **Upscale Modes & Hotkey:** Cycle between Crisp Pixel 1x (direct nearest-neighbor integer scaling), Scale2x Edge-Directed, and Smooth Bilinear filtering dynamically via <kbd>F7</kbd> or through the <kbd>F2</kbd> configuration menu.

### CRT Simulation Shader Pipeline (<kbd>F6</kbd>)
* **Vintage Display Emulation:** OpenGL 3.3 GLSL post-processing shader faithfully reproducing the visual aesthetic of a curved vintage CRT monitor.
* **Scanlines, Triads & Bloom:** Features subtle horizontal 200-line scanlines, aperture grille phosphor triad masks, gentle radial glass curvature, corner vignette shading, and celestial core glow/bloom within a strict 1.0 ms GPU render budget. Toggle dynamically via <kbd>F6</kbd> or through the <kbd>F2</kbd> menu.

### Sub-Pixel Geometry & Antialiased Point Distribution
* **Polymap Sub-Pixel Rasterization:** Textured terrain, planetary surfaces, mountain peaks, and interior cabin structures retain floating-point vertex coordinates, using continuous edge slopes, sub-pixel edge pre-stepping, and scanline rounding. Completely eliminates polygon crawling and vertex wobbling during 3D movement.
* **Continuous Barycentric Poly3d:** Flat-shaded polygons (cockpit bezel, target reticles, radar compass) utilize sub-pixel half-pixel barycentric evaluation.
* **Bilinear Celestial Points:** Single-pixel stars and distant planets interpolate brightness across a 2x2 pixel footprint based on fractional sub-pixel coordinate offsets (`far_pixel_at`), smoothing star travel when panning.
* **Fidelity Toggle:** Sub-pixel geometry mode can be toggled on/off in the <kbd>F2</kbd> menu with <kbd>G</kbd>. Legacy integer rasterization remains available as a baseline with 100% regression test hash parity.

### Display & Presentation Settings Persistence
* **User Configuration File (`display_settings.ini`):** Display preferences are automatically preserved in the platform configuration directory (`~/.config/noctis-iv-om/` on Linux, `%APPDATA%\Noctis IV OM\` on Windows, or `./config/` in portable mode).
* **Saved Preferences:** Preserves aspect ratio mode, upscale mode, CRT shader toggle, sub-pixel fidelity, fullscreen state, timewarp simulation rate, HUD text visibility, lens flare mode, and seamless border settings across sessions.
* **Atomic File Writes:** Safe atomic configuration writes prevent corrupted files upon unexpected shutdown. Settings are restored before window creation on startup.

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
