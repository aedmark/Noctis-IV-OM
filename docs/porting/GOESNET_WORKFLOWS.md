# GOESnet Workflow Validation

M5-W06 composes the native GOESnet pieces into player workflows through the
production application. All mutations use disposable copies of the pinned
catalogs; the repository data is never edited by a test.

## Covered workflows

The headless application fixture restores the seeded FELYSIA situation and
drives the same dispatcher and cartography commands used by the cockpit:

- `HELP`, unknown, malformed, ambiguous, and missing-object requests;
- DOS-confirmed `PAR MIRACLE` and `PAR FELYSIA` fields;
- remote-star and local-planet `ST` actions without an interchange file;
- `DL FELYSIA`, including the P15 planet-specific count of 238 notes;
- normal UI creation of a star label and a planet label, followed by `PAR`,
  `WHERE`, `SL`, and `ST` resolution and normal UI removal;
- `CAST`, `CAT`, `REP`, and `DELE`, with each read reopening the isolated guide
  and with protected-record replacement rejected.

The data and command tests add lower-level closure evidence: labels survive a
reopen, protected starmap and guide records cannot be changed, removal leaves
the rest of the starmap intact, missing and truncated inputs produce stable
statuses, read-only data rejects mutation, and a deliberately blocked temporary
replacement leaves the original guide byte-for-byte unchanged.

## Content identity

The bundled October 2023 content used for P22 is pinned by SHA-256:

| File | SHA-256 |
| --- | --- |
| `data/STARMAP.BIN` | `9fac3dd47c77127aba5f6f2fc9a1a8ea6f9b6577c6117c66bc8c0189948ebb20` |
| `data/GUIDE.BIN` | `e2d22f76383a8ac254f3bd6dd956faec69a47f080955b332cc0fbf8fb228b3b3` |

These checksums identify the accepted content version; the parsers and queries
remain responsible for structural and behavioral validation. The canonical
machine-readable copy is `data/CONTENT_MANIFEST.json`; M6-W04's
`tools/verify_content.py` checks and stages the package as documented in
`NIVPLUS_CONTENT.md`.

## Scope

This is deterministic Linux evidence through the production application, not
a claim that every historical catalog entry has been manually inspected.
Windows verification remains deferred to M7-W01 by ADR-0009. Broader damaged
save/catalog and interrupted-write matrices belong to M5-W07.
