# Noctis IV OM — Linux compatibility preview

Noctis IV OM is a hybrid of Noctis IV Plus and Noctis IV LR. This preview is
verified on 64-bit Ubuntu 24.04 using the X11 display backend. It can run from a
Wayland desktop through XWayland; native Wayland support is not included.

## Run

Keep the archive contents together. Open a terminal in this directory and run:

```sh
./nivlr
```

The extracted folder contains read-only game resources and default catalogs.
Player files live outside it:

- saves and catalogs: `$XDG_DATA_HOME/noctis-iv-om/data`, or
  `~/.local/share/noctis-iv-om/data` when `XDG_DATA_HOME` is unset;
- screenshots and Moviemaker decks: the neighboring `gallery/` and `movies/`
  directories;
- configuration: `$XDG_CONFIG_HOME/noctis-iv-om`, or
  `~/.config/noctis-iv-om` when `XDG_CONFIG_HOME` is unset.

The folders and clean catalog copies are created on first normal launch. The
archive therefore does not need to be installed in a writable location. Do not
run the game directly from inside the compressed archive.

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

## Preview status

This is a compatibility-test build, not the finished 1.0 release. Please keep
the JSON Lines output from `./nivlr --diagnostics` with any startup report.
When this build is assigned to a focused test, follow the short route in
`COMMUNITY_TESTING.md` and use its linked compatibility-report form. Read
`KNOWN_ISSUES.md` for the accepted cross-platform surface-presentation
difference and current preview limits.

Noctis IV was created by Alessandro Ghignola. This project preserves work from
Noctis IV Plus, Noctis IV LR, and their contributors. See `CONTRIBUTORS.md`,
`LICENSE`, `WTOF-LICENSE.md`, and `THIRD_PARTY_NOTICES.md` in this archive.
