# Noctis IV OM — Linux 3.2.0 Release

Noctis IV OM is a hybrid of Noctis IV Plus and Noctis IV LR. This 3.2.0 release is
verified on 64-bit Ubuntu 24.04+ using native Wayland and X11 display backends.
It automatically selects native Wayland on Wayland compositors (GNOME, KDE Plasma)
and falls back to X11 on X11 desktops.

## Run

Keep the archive contents together. Open a terminal in this directory and run:

```sh
./nivlr
```

This release runs in **portable mode by default**. Player files,
saves, mutable catalogs, screenshots, and Moviemaker decks are kept
self-contained within the extracted game directory:

- saves and catalogs: `./data/` beside `nivlr`;
- screenshots and Moviemaker decks: `./gallery/` and `./movies/`;
- configuration: `./config/`.

The runtime folders and clean catalog copies are created on first normal launch.
Ensure the extracted game folder is placed in a writable location. Do not run
the game directly from inside the compressed archive.

If you prefer system user directories instead:
Run `./nivlr --system-user-data` to store player data in
`$XDG_DATA_HOME/noctis-iv-om/data` (`~/.local/share/noctis-iv-om/data`) and
configuration in `$XDG_CONFIG_HOME/noctis-iv-om` (`~/.config/noctis-iv-om`).
You can also specify a custom profile path using `--user-data-dir DIRECTORY`.

For a headless installation check, run `./nivlr --diagnostics`.
To reset player saves, flight logs, and bookmarks and restore default catalogs, run `./nivlr --reset-data`.
To export player discoveries to a starmap packet, run `./nivlr --export-starmap [PATH]`.
To import and merge an external starmap packet, run `./nivlr --import-starmap <PATH>`.
To dry-run inspect a packet without modifying catalogs, run `./nivlr --validate-starmap <PATH>`.

## Bring forward an older portable profile

Keep the old folder as a backup, then run this from the new extracted folder:

```sh
./nivlr --prepare-user-data --migrate-from "/path/to/old Noctis folder"
```

The importer copies recognized saves, catalogs, screenshots, and movie decks
into the new user-data location. It never deletes source files or overwrites a
file already present at the destination. Diagnostics show all resolved paths.
For a deliberately portable or test profile, `--user-data-dir DIRECTORY`
selects an explicit root containing `data/`, `gallery/`, `movies/`, and
`config/`.

## Ubuntu runtime libraries

This archive contains the game and its data, not operating-system graphics,
windowing, or audio libraries. On a minimal Ubuntu 24.04 installation, install:

```sh
sudo apt-get update
sudo apt-get install -y libasound2t64 libx11-6 libxext6 libxrandr2 libxi6 \
  libgl1 libglu1-mesa libxcursor1 libxinerama1 libsm6 libice6
```

These are runtime libraries. The separate `-dev` packages are needed only when
building the game from source.

## Controls and Audio

- **Fullscreen:** Press `F11` or `Alt+Enter` to toggle fullscreen mode.
- **Aspect Ratio:** Press `F8` to cycle aspect ratio (4:3 CRT authentic, 16:10 square pixels, 16:9 stretch).
- **Upscaling:** Press `F7` to cycle upscaling mode (Crisp Pixel 1x, Scale2x Edge-Directed, Smooth Bilinear).
- **CRT Shader:** Press `F6` to toggle vintage monitor CRT simulation (scanlines, aperture grille, barrel curve, bloom).
- **Sub-Pixel Fidelity:** Press `F2` then `G` to toggle sub-pixel geometry rasterization and antialiased stars, eliminating 3D mesh wobble and jitter.
- **Coronal Flares & Limb Darkening:** Press `F2` then `E` to cycle stellar coronal flares (AUTHENTIC, REALISTIC, VIBRANT). Realistic limb darkening and Planckian color grading across all 12 stellar classes.
- **Settings Persistence:** Display aspect ratios, upscale filters, CRT shader, sub-pixel fidelity, coronal flares, fullscreen, and timewarp settings persist across launches in `display_settings.ini`.
- **Timewarp & Timelapse:** Press `T` or `Shift+S` (in space or on planetary surfaces) to toggle timewarp. Press `[` / `]` or drag the on-screen HUD slider to adjust the simulation rate (1x to 5000x).
- **Suit Torch:** Press `L` while exploring planetary surfaces to toggle headlamp. Features distance attenuation fading out before distant horizon mountains and soft highlight compression.
- **Suit Visor:** Press `Page Up` / `Page Down` to raise/lower helmet visor.
- **Waypoint Bookmarks & Starmap:** Press `J` in cockpit to jump directly to GOESnet bookmarks (`BM`). Press `J` while exploring a planetary surface to drop an instant GPS surface waypoint.
- **Explorer's Visor HUD:** Press `V` on planetary surfaces to cycle Visor telemetry (Standard, Explorer Telemetry with 360° cardinal compass tape, digital heading, planetary coordinates/elevation, and Lander Return Beacon range/direction, or Minimal).
- **Controls & Gamepad Menu:** Press `F2` then `Tab` or `C` to open Controls Options. View gamepad connection status, toggle mouse pitch inversion (`I`), toggle haptic rumble (`R`), or adjust sensitivity and deadzones. Settings persist in `controls.ini`.
- **Gamepad Flight & Traversal:** Dual-stick analog movement and camera look with deadzone filtering. Left/Right triggers (`LT`/`RT`) accelerate/decelerate; bumpers (`LB`/`RB`) roll ship or boost jetpack; `B` button safely cancels/deselects without quitting. Dual-motor rumble for thrusters, atmospheric buffeting, and landing.
- **Image Archive & PNG Export:** Press `F4` to open the Image Archive Viewer. Press `F4` to cycle export format (TARGA/PNG) and press `D` to export the snapshot or panorama directly to your Downloads folder.
- **Moviemaker Video Export:** Record in-flight movie decks and export directly to WebM/MP4 video files. Preview recorded movie reels on the cockpit bulkhead screen.
- **Ambient Music & Generative Soundscapes:** Press `F2` then `Tab` or `A` to open Audio Options. Press `G` to cycle music mode (GENERATIVE, RECORDED, HYBRID, OFF); press `N` / `P` to skip tracks. Features a 4-voice procedural generative melody and chord synthesizer tailored to all 12 stellar classes.
- **Audio Volume & Categories:** 6-channel mixer (Master, Music, Cabin, Propulsion, Weather, Foley). Use `1`–`6` to select category, `+` / `-` to step volume, or click/drag sliders. Volumes persist in `config.ini`.
- **Audio Mute:** Press `F9` or `Ctrl+M` (or `M` in Audio Options) to toggle procedural audio mute.
- **Cursor Unlock & Screenshots:** Press `F10` to unlock/lock mouse cursor. Unlocking immediately freezes camera and player movement with zero drift or inertia, ideal for external screenshot utilities.
- **Jetpack:** Press `Space` to burst thrusters while airborne on low-gravity worlds.

## Release status

This is the 3.2.0 Browser Moviemaker & WebM Release. It repairs scaled capture
indicators, exports clean recorded decks as native-resolution WebM in browsers,
adds direct web export from the projector, F3 panel, and GOESnet, and documents
the complete playback control set. Browser movie-deck shortcuts no longer change
page zoom. It includes the deterministic engine, save compatibility, and renderer
safety work from the 3.1.0 Engine Reliability Release.
Please keep the JSON Lines output from `./nivlr --diagnostics` with any startup report. Read
`KNOWN_ISSUES.md` for compiler-specific presentation details, and consult
`TROUBLESHOOTING.md` for solutions to common display, controls, and recovery
questions.

Noctis IV was created by Alessandro Ghignola. This project preserves work from
Noctis IV Plus, Noctis IV LR, and their contributors. See `CONTRIBUTORS.md`,
`LICENSE`, `WTOF-LICENSE.md`, `THIRD_PARTY_NOTICES.md`, and `TROUBLESHOOTING.md` in
this archive.
