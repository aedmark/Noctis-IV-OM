# M7-W04 runtime paths and profile migration

Status: complete on Linux and Windows.

M7-W04 separates immutable installed material from mutable player files. The
game no longer depends on its launch directory and a packaged build no longer
needs permission to write beside the executable.

## Path contract

| Purpose | Linux | Windows |
| --- | --- | --- |
| Executable resources | `res/` beside `nivlr` | `res\` beside `nivlr.exe` |
| Default starmap and guide | `defaults/` beside `nivlr` | `defaults\` beside `nivlr.exe` |
| Saves, mutable catalogs, gallery, movies | `$XDG_DATA_HOME/noctis-iv-om`, falling back to `~/.local/share/noctis-iv-om` | `%LOCALAPPDATA%\Noctis IV OM` through the Windows Known Folder API |
| Configuration | `$XDG_CONFIG_HOME/noctis-iv-om`, falling back to `~/.config/noctis-iv-om` | `%APPDATA%\Noctis IV OM` through the Windows Known Folder API |

The player-data root contains `data/`, `gallery/`, and `movies/`. The
configuration directory is created and reported now; existing cockpit and NIV+
preferences remain versioned with `current.niv`, so W04 does not split a saved
game across two authoritative files.

Executable discovery uses `/proc/self/exe` on Linux and `GetModuleFileNameW` on
Windows, with `argv[0]` as a fallback. Resources therefore resolve correctly
when the game is launched from a shortcut, shell, or unrelated working
directory. Windows user roots use `SHGetKnownFolderPath`, with `LOCALAPPDATA`
and `APPDATA` environment fallbacks for reduced environments such as Wine.

## First run and migration

A normal launch creates the four mutable directories. Missing `STARMAP.BIN`
and `GUIDE.BIN` files are copied from immutable `defaults/`; an existing player
catalog is never replaced.

By default, the game also recognizes the older portable layout beside the
executable. Players moving from a different extracted folder can import it
without opening a window:

```text
nivlr --prepare-user-data --migrate-from OLD_DIRECTORY
```

The source directory may contain:

- `data/current.niv`, `current.bin`, `surface.niv`, or `surface.bin`;
- mutable `data/STARMAP.BIN`, `GUIDE.BIN`, or `guide-export.txt`;
- files and subdirectories under `gallery/` and `movies/`.

Migration copies only regular files into missing destinations. It recursively
merges gallery and movie directories, skips symbolic links, never overwrites a
collision, and never deletes or modifies the source. Repeating the operation is
safe. The native/legacy save-format upgrade remains a separate bounded process:
after a legacy file reaches the new profile, the existing M5 reader validates
it and writes native v1 without deleting the imported source.

`--user-data-dir DIRECTORY` selects an explicit profile root for portable,
testing, or recovery use. `NOCTIS_IV_OM_HOME` provides the same override for
automation. `NOCTIS_IV_OM_RESOURCE_DIR` is a diagnostic/test override and is
not part of ordinary player setup.

## Diagnostics and failure behavior

`--diagnostics` remains read-only. Its JSON Lines report now includes the
working directory, executable directory, resource directory, default-catalog
directory, user-data root, configuration directory, migration source, runtime
directory presence, and each relevant catalog/save candidate. Combining
`--diagnostics` with `--migrate-from` reports the candidate but explicitly does
not copy it.

A normal or `--prepare-user-data` launch stops before graphics if a required
directory cannot be created or a requested copy fails. The log reports copied
and preserved-file counts. A nonexistent explicit `--migrate-from` directory
is rejected instead of silently falling back to a clean profile.

## Evidence

The focused `runtime_paths` test covers XDG paths and fallbacks, Windows
Local/Roaming AppData policy, explicit profiles, clean catalog seeding,
colocated-profile import, nested gallery/movie migration, source preservation,
collision preservation, and idempotent replay.

The `runtime_paths_application` fixture invokes the real executable from an
unrelated working directory. It exercises the documented headless migration
command, exact catalog copying, save/catalog collision preservation, source
preservation, idempotent replay, resolved-path diagnostics, and rejection of a
missing explicit source.

All 40 tests pass with Clang, GCC, and Clang ASan/UBSan on Linux. The application
fixtures use explicit isolated profiles and continue to cover saves, restarts,
catalog edits, screenshots, Moviemaker, and surface journeys. A generated Linux
archive passes exact-content verification and headlessly creates a fresh
profile with both catalogs and all mutable directories.

Hosted runs `36866928901` and `36866929363` pass the complete Linux build/test
matrix and revised package verifier. Windows runs `36866929200` and
`36866929028` pass the required MSVC suite and revised ZIP verifier, including
resource/default placement beside the configuration-specific executable and
isolated Local/Roaming AppData overrides. These four runs use commit `2f0a1b2`
and close M7-W04.
