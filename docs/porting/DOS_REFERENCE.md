# DOS behavioral reference (M0-W05)

Pinned from this repository at Git commit
`5c46de934ef69130b5b672f9cc3de703558364cb` (the `Origins` commit).
These are **existing tracked files**, not new copies or a redistribution
clearance. The corresponding permission questions are in `PROVENANCE.md`.

| Role | Repository path | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| NIV+ DOS executable | `modules/NOCTIS.EXE` | 215744 | `5e64d532091c9be1f91d7e0bc57719df24020ba38b0662f225f65d3c55e579ac` |
| Starmap | `data/STARMAP.BIN` | 1202500 | `9fac3dd47c77127aba5f6f2fc9a1a8ea6f9b6577c6117c66bc8c0189948ebb20` |
| Guide | `data/GUIDE.BIN` | 4063588 | `e2d22f76383a8ac254f3bd6dd956faec69a47f080955b332cc0fbf8fb228b3b3` |
| Support data | `data/SUPPORTS.NCT` | 60776 | `495259a0e453c7a56ed5f0bdc2a5788c855ec788843c42666ada7ad437d4e4f3` |
| Globe map for source comparison | `source/GLOBES.MAP` | 22586 | `9a8dedfd7bdba5ded19625ba1187ddd4c0986dfe30d1e2bc13264bd1fc9af9fb` |
| Offset map for source comparison | `source/OFFSETS.MAP` | 7340 | `a868e8ccae5f1803fe141a8789cf9ca935a8513ac69953c29ceeedfcfe346ec6` |
| DOSBox-X configuration | `dosbox.conf` | 225 | `3da3ec703dad90829e854754586d735c8c97ab541d22e725c286b0f8cc151e2d` |

`source/SUPPORTS.NCT` is byte-identical to `data/SUPPORTS.NCT` at this
commit. `modules/NOCTIS.EXE` is an MZ DOS executable; its last recorded update
was Joris van de Donk's `62c27aa` (release 2.3, 2023-10-01). The two data
files' last recorded update was `3759857` (2023-10 starmap/guide). This pins
the *checked-in* reference, not an assertion that it is the latest release or
that every runtime file has been captured.

## Reproduction procedure

From a checkout at the pinned commit, verify the table with `sha256sum` and
`wc -c`. For a manual baseline run, install DOSBox-X separately; do not add
the emulator to this repository. From the repository root, use the documented
launch sequence (the `Launch.bash` and `Launch.bat` scripts encode it): mount
the repository root as drive `N:`, switch to `N:`, change to `modules`, then
start `NOCTIS.EXE` with `dosbox.conf`. Keep an untouched checkout for fixture
capture; the game may write state alongside its input files.

The checked configuration specifies `dynamic_x86`, `pentium_iii`, maximum CPU
cycles, and aspect correction. The local DOSBox-X executable reports version
`2026.08.31 SDL1`. The later F01A fixture validates an isolated launch and
capture path and records emulator version, host platform, startup steps,
output capture method, and initial state. Other scenarios still need captures.

This work item pins inputs and launch settings only. No DOS run or visual
comparison was performed for this record, and no new binary was committed.
The later [F01A ADELPHE](fixtures/F01A_ADELPHE/manifest.json),
[F01B JEHOVABOH](fixtures/F01B_JEHOVABOH/manifest.json), and
[F01C MIRACLE](fixtures/F01C_MIRACLE/manifest.json), and
[F01D NEW FELYSIA](fixtures/F01D_NEW_FELYSIA/manifest.json) captures record
isolated DOS runs and numeric comparisons against the modern port. The
[F02A FELYSIA parent capture](fixtures/F02A_FELYSIA_PARENT/manifest.json)
records DOS planet/star lookup output and the pinned starmap ID relationship;
the follow-on [F02B property capture](fixtures/F02B_FELYSIA_PROPERTIES/manifest.json)
records two local-target data runs and their native property/period match.
The [F02C surface capture](fixtures/F02C_FELYSIA_SURFACE/manifest.json) records
the `LQ 001:060` landing, decoded replay state, green surface palette, and the
matching native global/local surface seeds.
