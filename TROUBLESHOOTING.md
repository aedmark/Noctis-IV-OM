# Noctis IV OM Troubleshooting Guide

This guide covers common issues, diagnostics, controls, and recovery procedures
for players and developers running Noctis IV OM.

---

## 1. Quick Diagnostics

If you encounter unexpected behavior or want to verify your installation, run the
headless diagnostic preflight check:

### Linux

```sh
./nivlr --diagnostics
```

### Windows (PowerShell)

```powershell
.\nivlr.exe --diagnostics
```

The command outputs machine and environment details as JSON Lines without
opening a graphical window or modifying save files. Check that:
- `assets_present` is `true`.
- The resource directory (`res/`) is found and valid.
- The user-data, configuration, and defaults directories resolve to writable paths.

---

## 2. Display and Graphics Issues

### The game fails to start or crashes on window creation

1. **Verify OpenGL support:** Noctis IV OM requires OpenGL 3.3 or compatible
   hardware rasterization.
2. **Software rendering fallback (Linux):** If you are running on virtualized
   hardware, headless machines, or an incompatible GPU driver, force software
   rendering via Mesa:
   ```sh
   LIBGL_ALWAYS_SOFTWARE=1 ./nivlr
   ```
3. **Wayland sessions (Linux):** The Linux build includes native Wayland support
   alongside X11. It automatically connects natively on Wayland compositors (GNOME,
   KDE Plasma, etc.) and falls back to X11 on X11 sessions. If you ever need to
   force the X11/XWayland backend on a Wayland desktop, launch with:
   ```sh
   WAYLAND_DISPLAY="" XDG_SESSION_TYPE=x11 ./nivlr
   ```

### Windows SmartScreen warning

Because the preview packages are not code-signed with a commercial certificate,
Windows Defender SmartScreen may show a warning when launching `nivlr.exe`. Click
**More info**, then **Run anyway**. You can verify archive integrity using the
provided `.sha256` checksum before running.

### Graphical Smoke Test

To verify that the windowing and presentation layer functions properly without
requiring manual input:

```sh
# Linux
./nivlr --graphical-smoke

# Windows
.\nivlr.exe --graphical-smoke
```

This presents three frames and exits cleanly with return code 0.

---

## 3. Ship Controls and Gameplay Quirks

### Keyboard shortcuts (e.g. `5`, `7`, `F1`) are not responding

**The "Terminal Focus" Trap:**
If you recently used the GOESnet terminal or guide computer, keyboard focus may
still be captured by the terminal input buffer. When focused, numeric and letter
keys type into the terminal prompt rather than triggering flight shortcuts.

- **Solution:** Right-click anywhere on the terminal screen to deselect it and
  restore ship cockpit keyboard focus.

### The cockpit screen blinks off repeatedly / "POWER LOSS"

This is an authentic in-universe simulation mechanic, **not a bug**:
- If both current conventional power and lithium reserves drop to zero, the
  ship enters a low-power state. The onboard computer blinks off intermittently
  and the drive systems shut down.

**How to Recover:**
1. Quit the game normally (`Esc` or close window).
2. Launch once from the terminal with the `--standard-drive` flag:
   ```sh
   ./nivlr --standard-drive
   ```
   (or on Windows: `.\nivlr.exe --standard-drive`)
3. This restores conventional power and refills your lithium reserves to full
   without resetting your current location or save data.

### Mouse Look vs. Mouse Cursor

Press `Tab` to toggle between free-look camera mode and mouse cursor interaction.

---

## 4. Saves, Data Paths, and Migration

### Where are my saves and catalogs stored?

- **Portable Mode (Default for Release Packages):**
  Files are kept directly inside the game folder:
  - Saves & Catalogs: `./data/`
  - Screenshots: `./gallery/`
  - Moviemaker Decks: `./movies/`
  - Settings: `./config/`
- **System Mode:**
  - Linux: `$XDG_DATA_HOME/noctis-iv-om` (typically `~/.local/share/noctis-iv-om`)
  - Windows: `%LOCALAPPDATA%\Noctis IV OM`

To force system paths in a portable build, launch with:
```sh
./nivlr --system-user-data
```

To specify an explicit custom profile directory:
```sh
./nivlr --user-data-dir "/path/to/custom/profile"
```

### Migrating data from an older Noctis or DOS installation

To safely bring forward your existing starmap, guides, screenshots, and saves
without modifying or deleting the originals:

```sh
# Linux
./nivlr --prepare-user-data --migrate-from "/path/to/old/noctis"

# Windows
.\nivlr.exe --prepare-user-data --migrate-from "C:\Path\To\Old\Noctis"
```

The importer never overwrites existing destination files and never alters the
source directory.

---

## 5. Audio Status

### There is no sound

The current native port builds are **intentionally silent**. Alessandro
Ghignola's original procedural synthesizer and sound effects engine are scheduled
for integration in a subsequent update. Silence during gameplay is expected and
does not indicate an audio driver defect.

---

## 6. Submitting a Bug Report

If you encounter a reproducible crash or defect not covered above:

1. Run diagnostics and save the output:
   ```sh
   ./nivlr --diagnostics > noctis-diagnostics.txt
   ```
2. Open an issue on GitHub:
   <https://github.com/aedmark/Noctis-IV-OM/issues>
3. Include:
   - Operating system and desktop environment.
   - Graphics hardware and driver version.
   - The contents of `noctis-diagnostics.txt`.
   - Exact steps to reproduce the issue.
