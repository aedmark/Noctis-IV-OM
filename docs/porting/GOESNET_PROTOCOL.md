# M5-W04 GOESnet command and result contract

Status: complete on Linux. This specifies the native boundary for M5-W05 and
the workflow evidence required by M5-W06; it does not claim the commands are
implemented yet.

## Compatibility boundary

The native port preserves the onboard command environment, pinned starmap and
guide semantics, and player-visible results. It does not preserve the DOS
mechanism. Native code must never compose a shell command, launch an executable
from `modules/`, redirect process output, or exchange state through
`current.bin` or `COMM.BIN`.

Historical `source/*.CPP`, inactive `modern/src/Old/` translations, pinned data,
and accepted raw DOS outputs are compatibility evidence. They are not safe
parsers and are not compiled into the native application.

## Command-line grammar

The command screen owns at most 83 entered characters plus its visible `_`
cursor. The protocol removes exactly one final cursor, trims outer ASCII spaces,
uppercases ASCII letters, and changes `"` to `'`, matching the inherited console.
An empty line performs no operation.

```text
request       = command [ SP argument ]
command       = 1*ASCII-letter
argument      = 1*(allowed-character)
SP            = one or more ASCII spaces
```

Allowed bytes are ASCII 32 through 90 plus `_`, except `$`, `&`, `<`, and `>`.
Control bytes, non-ASCII input, and commands longer than 83 characters are
rejected before dispatch. There is no quoting or escaping layer: spaces remain
part of the argument and colons are interpreted only by the selected command.
Dispatch uses the complete command token; prefixes do not match.

The parser returns `ok`, `empty`, `too_long`, `invalid_character`,
`unknown_command`, `missing_argument`, or `unexpected_argument`. No parse error
may read data or change game state.

## Registry and first-release scope

| Command | Argument | Native disposition | Observable purpose |
| --- | --- | --- | --- |
| `CLR` | none | Resident | Clear output and reset its scroll position |
| `HELP` | optional command | Required | List commands or show command syntax |
| `PAR` | object[`:`range] | Required | Report star coordinates or a planet's parent coordinates |
| `WHERE` | planet | Required | Report a labelled planet's parent star |
| `ST` | object[`:`range] | Required | Resolve a labelled object and request travel |
| `DL` | object[`:`range] | Required | List planets or moons and guide-note counts |
| `CAT` | object[`:`first`..`last`] | Required | Read guide records for an object |
| `CAST` | object`:`note | Required | Append a guide note |
| `REP` | object`:`record`:`note | Required | Replace an unprotected guide note |
| `DELE` | object[`:`first`..`last`] | Required | Remove unprotected guide notes |
| `SL` | [range] | Required | List labelled stars, optionally within range |
| `PRI` | object[`:`first`..`last`] | Native export | Export selected guide text to a native text file |
| `CLEAN` | none | Required | Compact starmap file by erasing tombstones and reclaiming space |
| `INBOX` | [file / `CHECK`] | Required | Import and merge starmap exchange packet with collision validation |
| `OUTBOX` | [`BIN` / `JSON`] | Required | Export custom star and planet discoveries as shareable packet |

`REPAIR.EXE`, `txt.exe`, and `wri.exe` are utilities rather than onboard
commands. Bundled `HELP.com` becomes native `HELP` behavior for ledger item P14;
the DOS binary is never launched. `CLEAN`, `INBOX`, and `OUTBOX` provide native,
validated modern sharing with strict integrity and collision checks.

Object keys contain 1–20 characters. Matching is case-insensitive after console
normalization, ignores tombstoned records, and retains historical prefix search:
one exact or unique prefix resolves, no matches report `not_found`, and multiple
prefixes report `ambiguous` with candidates. W05 parses colons and numeric ranges
with checked conversion; malformed, reversed, negative, or overflowing ranges
are usage errors and never silently become zero.
Native ranged galaxy scans accept 3–100 sectors per axis. Larger historical
values are rejected as unsafe cubic interactive work rather than silently
falling back to another range.

## Native result shape

Every dispatch produces one `GoesResult` with three independent fields:

- `status`: `ok`, `usage_error`, `unavailable`, `corrupt_data`, `not_found`,
  `ambiguous`, `rejected`, `write_failed`, or `unsupported`;
- `action`: none, clear output, set a remote target, set a local target, record
  a catalog change, or report a completed export;
- `cells`: owned display bytes containing complete 21-byte rows.

Rows are truncated to 21 bytes and space-padded to exactly 21 bytes. The stream
has no cursor or required NUL terminator. `&`, `*`, `$`, `[`, and `]` remain
display markup where historical output uses them. Empty output is valid only for
`CLR`; all other results include a useful message. The 21-by-7 viewer pages this
stream without newlines or a temporary file. W05 connects this owned stream
directly to the cockpit viewer; `GOESfile.txt` is no longer created.

Status and action are authoritative; display text is never reparsed to trigger
travel or detect success. A target action carries validated typed data in the
W05 result extension and applies only after complete success. Guide mutation
also commits only after bounded parse, validation, and replacement write.
Failures leave target, catalog, save state, prior output, and scroll unchanged,
except that the failure message becomes the new output after dispatch.

## Data and failure rules

`STARMAP.BIN` and `GUIDE.BIN` are untrusted even when expected checksums are
documented. W05 readers use explicit little-endian fields, validate the four-byte
consolidated boundary, require complete fixed records, cap counts from file size,
and reject truncated or impossible files. They never reinterpret runtime structs
over bytes or partially apply a damaged file.

| Condition | Status | State outcome |
| --- | --- | --- |
| Missing required data | `unavailable` | No state change |
| Invalid boundary, record, or truncated read | `corrupt_data` | No state change |
| No matching live object | `not_found` | No state change |
| More than one prefix match | `ambiguous` | No change; candidates returned |
| Invalid syntax or range | `usage_error` | No state change |
| Protected note or disallowed mutation | `rejected` | No state change |
| Replacement/export failure | `write_failed` | Original retained |
| Recognized obsolete command | `unsupported` | Explanation; no state change |

Catalog writes use a temporary beside the destination, flush and close it, then
replace the destination. Failure retains the original. `CLR` alone intentionally
discards prior output without producing rows.

## W05 and W06 acceptance handoff

M5-W05 implements the required commands behind this registry and has removed
`system()` plus legacy save/communication-file interoperability. Its focused
reader, command, and application tests are described in `GOESNET_NATIVE.md`.

M5-W06 exercises through the production application at minimum:

1. `HELP`, unknown, and malformed commands;
2. accepted DOS text fields for `PAR MIRACLE`, `PAR FELYSIA`, plus
   ambiguous/not-found paths;
3. `ST` remote-star and local-planet actions without `COMM.BIN`;
4. `DL` star and planet note counts, including P15;
5. `CAT`, `CAST`, `REP`, and `DELE` on an isolated copy with reopen verification
   and protected-record rejection;
6. normal UI label creation followed by `PAR`, `WHERE`, `SL`, and resolution;
7. missing, truncated, corrupt, and read-only outcomes without partial change.

Pinned 2023-10 data checksums remain content-version evidence for P22. Mutating
tests use isolated copies. Windows stays deferred to M7-W01 by ADR-0009.
