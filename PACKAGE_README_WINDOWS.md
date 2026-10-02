# Noctis IV OM — Windows compatibility preview

Noctis IV OM is a hybrid of Noctis IV Plus and Noctis IV LR. This unsigned
preview is built for 64-bit Windows with Visual Studio 2022. Hosted Windows CI
verifies compilation, packaging, diagnostics, and the platform-independent test
suite. The packaged game has also completed its short graphics check on native
Windows and under Wine. Its complete Release fixture suite enforces the
published Windows presentation baselines.

## Run

Extract the complete ZIP and keep its contents together. Open PowerShell in the
extracted directory and run:

```powershell
.\nivlr.exe
```

The extracted folder contains read-only game resources and default catalogs.
The game writes saves, mutable catalogs, screenshots, and Moviemaker decks
under `%LOCALAPPDATA%\Noctis IV OM`, and reserves
`%APPDATA%\Noctis IV OM` for configuration. These locations are created on
first normal launch, so the extracted game folder does not need to be writable.
Do not run the game directly from inside the ZIP.

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

## Preview status

This is a compatibility-test build, not the finished 1.0 release. The Microsoft
C/C++ runtime is linked into the executable, so the ZIP does not require a
separate Visual C++ Redistributable installation. Please keep the JSON Lines
output from `--diagnostics` with any startup report.
When this build is assigned to a focused test, follow the short route in
`COMMUNITY_TESTING.md` and use its linked compatibility-report form. Read
`KNOWN_ISSUES.md` for the accepted Linux/Windows surface-presentation
difference and current preview limits.

Noctis IV was created by Alessandro Ghignola. This project preserves work from
Noctis IV Plus, Noctis IV LR, and their contributors. See `CONTRIBUTORS.md`,
`LICENSE`, `WTOF-LICENSE.md`, and `THIRD_PARTY_NOTICES.md` in this archive.
