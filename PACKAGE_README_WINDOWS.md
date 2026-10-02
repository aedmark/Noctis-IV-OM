# Noctis IV OM — Windows 1.4.0 Release

Noctis IV OM is a hybrid of Noctis IV Plus and Noctis IV LR. This 1.4.0 release is
built for 64-bit Windows. The Microsoft C/C++ runtime is statically linked so the
package runs out-of-the-box without requiring separate Visual C++ Redistributable
installations.

## Run

Extract the complete ZIP and keep its contents together. Open PowerShell in the
extracted directory and run:

```powershell
.\nivlr.exe
```

This release runs in **portable mode by default**. Player files,
saves, mutable catalogs, screenshots, and Moviemaker decks are kept
self-contained within the extracted game directory:

- saves and mutable catalogs: `.\data\` beside `nivlr.exe`;
- screenshots and Moviemaker decks: `.\gallery\` and `.\movies\`;
- configuration: `.\config\`.

The runtime folders and clean catalog copies are created on first normal launch.
Ensure the extracted folder is placed in a writable directory. Do not run the
game directly from inside the ZIP archive.

If you prefer system user directories instead:
Run `.\nivlr.exe --system-user-data` to store saves under
`%LOCALAPPDATA%\Noctis IV OM` and configuration under `%APPDATA%\Noctis IV OM`.
You can also specify a custom profile path using `--user-data-dir DIRECTORY`.

For a headless installation check, run:

```powershell
.\nivlr.exe --diagnostics
```

For the short graphics acceptance check, run:

```powershell
.\nivlr.exe --graphical-smoke
```

That command opens the real game window, presents three frames, and closes on
its own. It does not require keyboard or mouse input. Windows may show a
SmartScreen warning because this preview is not code-signed.

## Bring forward an older portable profile

Keep the old folder as a backup, then run this from the new extracted folder:

```powershell
.\nivlr.exe --prepare-user-data --migrate-from "C:\Path\To\Old Noctis Folder"
```

The importer copies recognized saves, catalogs, screenshots, and movie decks
into the new user-data location. It never deletes source files or overwrites a
file already present at the destination. Diagnostics show all resolved paths.
For a deliberately portable or test profile, `--user-data-dir DIRECTORY`
selects an explicit root containing `data\`, `gallery\`, `movies\`, and
`config\`.

## Controls and Audio

- **Fullscreen:** Press `F11` or `Alt+Enter` to toggle fullscreen mode.
- **Aspect Ratio:** Press `F8` to cycle aspect ratio (4:3 CRT authentic, 16:10 square pixels, 16:9 stretch).
- **Timewarp & Timelapse:** Press `T` or `Shift+S` (in space or on planetary surfaces) to toggle timewarp. Press `[` / `]` or drag the on-screen HUD slider to adjust the simulation rate (1x to 5000x).
- **Suit Torch:** Press `L` while exploring planetary surfaces to toggle headlamp.
- **Suit Visor:** Press `Page Up` / `Page Down` to raise/lower helmet visor.
- **Audio Mute:** Press `F9` or `Ctrl+M` to toggle procedural audio mute.
- **Jetpack:** Press `Space` to burst thrusters while airborne on low-gravity worlds.

## Release status

This is the 1.4.0 Celestial & Display Update release featuring authentic CRT aspect ratio
cycling, fullscreen toggle, window resizing, real-time planetary axial rotation, and
an interactive timewarp multiplier slider HUD. The Microsoft C/C++ runtime is
statically linked into the executable, so the ZIP does not require a separate
Visual C++ Redistributable installation. Please keep the JSON Lines output from
`--diagnostics` with any startup report. Read `KNOWN_ISSUES.md` for
compiler-specific presentation details, and consult `TROUBLESHOOTING.md` for
solutions to common display, controls, and recovery questions.

Noctis IV was created by Alessandro Ghignola. This project preserves work from
Noctis IV Plus, Noctis IV LR, and their contributors. See `CONTRIBUTORS.md`,
`LICENSE`, `WTOF-LICENSE.md`, `THIRD_PARTY_NOTICES.md`, and `TROUBLESHOOTING.md` in
this archive.
