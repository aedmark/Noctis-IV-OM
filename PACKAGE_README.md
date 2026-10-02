# Noctis IV OM — Linux 1.7.0 Release

Noctis IV OM is a hybrid of Noctis IV Plus and Noctis IV LR. This 1.7.0 release is
verified on 64-bit Ubuntu 24.04+ using the X11 display backend. It can run from a
Wayland desktop through XWayland; native Wayland support is not included.

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
- **Settings Persistence:** Display aspect ratios, upscale filters, CRT shader, sub-pixel fidelity, fullscreen, and timewarp settings persist across launches in `display_settings.ini`.
- **Timewarp & Timelapse:** Press `T` or `Shift+S` (in space or on planetary surfaces) to toggle timewarp. Press `[` / `]` or drag the on-screen HUD slider to adjust the simulation rate (1x to 5000x).
- **Suit Torch:** Press `L` while exploring planetary surfaces to toggle headlamp.
- **Suit Visor:** Press `Page Up` / `Page Down` to raise/lower helmet visor.
- **Image Archive:** Press `F4` in the Stardrifter (or type `GALLERY` / `VIEW n` on the GOES console) to browse past snapshots and panoramas. `Left`/`Right` browse, `Z` zooms and pans panoramas, `Esc` closes.
- **Audio Mute:** Press `F9` or `Ctrl+M` to toggle procedural audio mute.
- **Jetpack:** Press `Space` to burst thrusters while airborne on low-gravity worlds.

## Release status

This is the 1.7.0 Web Update release. The complete game is now also playable in the
browser at https://aedmark.github.io/Noctis-IV-OM/play/, and quick key taps shorter than
one frame are no longer missed. Please keep
the JSON Lines output from `./nivlr --diagnostics` with any startup report. Read
`KNOWN_ISSUES.md` for compiler-specific presentation details, and consult
`TROUBLESHOOTING.md` for solutions to common display, controls, and recovery
questions.

Noctis IV was created by Alessandro Ghignola. This project preserves work from
Noctis IV Plus, Noctis IV LR, and their contributors. See `CONTRIBUTORS.md`,
`LICENSE`, `WTOF-LICENSE.md`, `THIRD_PARTY_NOTICES.md`, and `TROUBLESHOOTING.md` in
this archive.
