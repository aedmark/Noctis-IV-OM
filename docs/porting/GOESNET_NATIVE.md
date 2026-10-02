# M5-W05 Native GOESnet commands

Status: complete on Linux. Windows remains deferred by ADR-0009. Full combined
label/catalog/player workflows remain assigned to M5-W06.

## Outcome

The onboard console now dispatches native C++ components. It no longer builds a
shell command, launches anything in `modules/`, writes a raw situation for a
child process, reads `COMM.BIN`, redirects stdout to `GOESfile.txt`, or reparses
files to discover a target. Results are owned 21-column cell streams, and `ST`
returns a typed remote or local target action which the flight loop applies.

`freeze()` now emits only `current.niv`. A legacy `current.bin` remains accepted
as an import source when native state is absent, but normal saves never create
or update one. The output screen pages native memory directly.

## Native components

- `goesnet_protocol` owns parsing, command metadata, result status/actions, and
  fixed-row formatting.
- `goesnet_data` performs capped, record-aligned little-endian reads of the
  starmap and guide. It validates consolidated boundaries, preserves historical
  blank names and negative ordinals safely, ignores tombstones, and identifies
  protected versus user-added records.
- `goesnet_commands` implements `HELP`, `PAR`, `WHERE`, `ST`, `DL`, `CAT`,
  `CAST`, `REP`, `DELE`, `SL`, `PRI`, and `CLR`. `CLEAN`, `INBOX`, and `OUTBOX`
  return an explicit retired-tool result and never invoke DOS code.

Object lookup retains exact-name priority followed by unique-prefix matching.
Procedural coordinates and systems reuse the tested native galaxy, star, and
planet generators. `DL` counts notes against the selected object's ID, including
the P15 planet-not-star correction. Ranged searches accept 3–100 sectors per
axis; larger historical values are rejected because their cubic scans are not a
safe interactive request.

Guide notes are fixed 84-byte records: little-endian subject ID plus a bounded
76-byte message. Appends, replacements, and deletions rewrite a complete
validated copy beside the destination, flush and close it, then replace the
original. Consolidated records cannot be replaced or deleted. Tests mutate only
isolated copies of the pinned guide.

The pinned `STARMAP.BIN` and `GUIDE.BIN` initialize missing runtime files beside
the built executable. Rebuilding does not overwrite an existing writable copy
or its user notes. Release checksums and provenance remain governed by
`DOS_REFERENCE.md`, `PROVENANCE.md`, and ADR-0007.

## Evidence and limits

`goesnet_data` loads both pinned files, verifies MIRACLE and FELYSIA identities,
rejects truncation, and reopens an isolated guide after append, replacement,
protected-record handling, and deletion. `goesnet_commands` checks HELP,
unknown and retired commands, DOS-backed `PAR MIRACLE`/`PAR FELYSIA`,
`WHERE FELYSIA`, ranged `SL`, typed remote/local `ST`, P15 note counts, and the
complete CAT/CAST/REP/DELE mutation sequence.

`goesnet_application` migrates the tracked console-pose save, runs native PAR
and ST through the production application, and proves that the resulting save
is native while shell/output/communication artifacts are absent. M5-W06 still
owns the broader combined workflows and normal UI label round trip; this work
does not claim those yet.
