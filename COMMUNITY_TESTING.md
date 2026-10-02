# Noctis IV OM compatibility test

Thank you for testing Noctis IV OM. This is a focused preview test, not a
request to play every corner of the game. A useful report takes about 20–30
minutes and tells us whether the same build behaves correctly on real Linux
and Windows computers.

Use only the archive and SHA-256 checksum linked by the test invitation. Keep
the archive unchanged, extract it into a new folder, and do not import an
existing profile until the clean-start checks are complete. The game stores
player files in the OS user-data area, not inside the extracted package.

## Before playing

1. Record the archive filename and compare it with its supplied SHA-256 file.
2. Record your operating-system version, CPU, graphics card, and graphics
   driver. Linux testers should also record whether the session is X11 or
   Wayland/XWayland.
3. Run the headless diagnostic command from the extracted directory and save
   all of its output:

   Linux:

   ```sh
   ./nivlr --diagnostics 2>&1 | tee noctis-diagnostics.txt
   ```

   Windows PowerShell:

   ```powershell
   .\nivlr.exe --diagnostics 2>&1 | Tee-Object noctis-diagnostics.txt
   ```

The report contains paths and basic machine information. Read it before
sharing it. It should not contain save data or account credentials.

## Core route

Please note the result of each numbered check, even if it is simply “pass.” If
a crash or data-loss problem occurs, stop and report it instead of continuing.

1. **Clean start:** launch from the fresh extraction. Confirm that the game
   opens and accepts keyboard and mouse input. This preview does not implement
   game audio; silence is expected and is not a test failure.
2. **Cockpit:** visit the main onboard computer pages and open contextual F1
   help. Toggle one F2 visual option and confirm that it changes the display.
   If the terminal repeatedly blinks off, check the ship's power: this is the
   inherited power-loss warning when both current power and lithium reserves
   are depleted, not a display failure. To recover a test save, quit normally
   and launch once with `--standard-drive`; this restores full conventional
   power and lithium without replacing the rest of the save.
3. **Space flight:** select a remote target. If you used the GOESnet terminal,
   right-click its screen to deselect it before using cockpit shortcuts. Press
   `5` to select Flight Control, then `7` to start Vimana travel. Cancel or
   complete the flight and confirm that steering and speed controls remain
   responsive. Number keys entered while a terminal is selected are terminal
   input, not cockpit commands.
4. **Planet visit:** approach a landable planet, land, walk away from the
   capsule, return, and lift back to the ship.
5. **Catalogs:** use GOESnet to look up a star or planet. Add and remove a
   personal label or guide note, then check that the change survives a restart.
6. **Persistence:** change a cockpit preference, save, quit normally, relaunch,
   and confirm that the position and preference return.
7. **Capture:** take an ordinary screenshot. If time permits, record a short F3
   Moviemaker deck, pause and resume it, then stop. Confirm that image files
   were created and that an existing deck is not overwritten.

The invitation may assign a narrower scenario or a known Noctis location in
addition to this route. Exact visual comparisons belong to the assigned
scenario; visual impressions alone do not establish procedural compatibility.

## Optional legacy-data route

Only do this when the invitation explicitly asks for migration coverage. Work
from a copy of your old data and retain an untouched backup. Never make the
preview your only copy of a long-lived save, starmap, or guide.

Record the source game/version when known, the original filename and size, what
the preview reported, and whether the migrated game state was correct. Do not
publish a personal save or catalog unless you are comfortable sharing its
contents.

## Report a result

Use the repository's **Community compatibility report** issue form:

<https://github.com/aedmark/Noctis-IV-OM/issues/new?template=compatibility-report.yml>

One report per machine and archive is easiest to evaluate. Attach
`noctis-diagnostics.txt`; include the checklist results and exact reproduction
steps for failures. Screenshots are welcome for display problems. Do not attach
private saves, crash dumps, or catalogs without reviewing their contents.

These are release-blocking findings during the focused test:

- the package cannot start on an in-scope system;
- a crash or hang occurs on the core route;
- saving, migration, or catalog editing loses or corrupts player data;
- the deterministic universe disagrees with a confirmed compatibility fixture;
- controls make the core route impossible to complete.

Other defects and usability problems are still valuable and will be triaged
for either the preview, the 1.0 stabilization milestone, or a documented known
issue.
