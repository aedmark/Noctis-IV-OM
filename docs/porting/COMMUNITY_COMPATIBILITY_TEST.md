# M7-W06 focused community compatibility test

Status: corrected replacement pair pinned; complete maintainer routes are
required before recruitment opens.

M7-W06 validates the packaged game on ordinary player machines after M7-W04
has finalized configuration/data paths and M7-W05 has published the
cross-platform comparison and known differences. It does not replace the
automated suite or use community impressions as deterministic evidence.

## Test boundary

The campaign uses one named Linux archive and one named Windows ZIP produced by
CI from the same commit. Invitations must include each archive's SHA-256 value,
the commit, the test dates, and a link to
`COMMUNITY_TESTING.md` inside the package.

The core route covers clean startup, cockpit UI/help, space flight, one complete
landing/lift cycle, GOESnet and personal catalog edits, save/restart, screenshot
capture, and a short Moviemaker deck. Legacy-data migration is a separately
assigned route so no tester is encouraged to risk their only save.

## Replacement candidate pair

Both corrected candidates were built from commit `7e764da` on 2026-10-01:

| Platform | Workflow run | Archive SHA-256 |
| --- | --- | --- |
| Linux x86-64 | `36873954692` | `843c8729d079d28e0ac805a71d3225ecab14171c1e2a20fde7b72c08a43e59c9` |
| Windows x86-64 | `36873954600` | `586d016d4e51dee23035bc8b711b74f0141fd448659b4b4a7ab1c348a4a4da05` |

Both included checksum files pass independent verification after artifact
download. The Linux workflow passed all 40 tests, exact package verification,
and its extracted-package graphical smoke. The Windows workflow passed all 40
non-graphical Release tests, exact package verification, and the
self-contained Microsoft-runtime check. Gates 1–3 are satisfied for this pair;
the complete maintainer route on each remains gate 4.

## Withdrawn candidate pair

Both candidates were built from commit `6617d0a` on 2026-10-01:

| Platform | Workflow run | Archive SHA-256 |
| --- | --- | --- |
| Linux x86-64 | `36869013197` | `f1397a195e44b94f29d8dc188f29fc7d70defa29424c2474b8fec5772c625b35` |
| Windows x86-64 | `36869013306` | `75b54d47c7a68610f6bef06623255bbfed45f2e8241eef72289357017f651a06` |

The Windows candidate passes all 40 non-graphical Release tests using the
published MSVC baselines. Both workflows verify their archive checksum,
extracted layout, diagnostics, isolated profile initialization, and packaged
tester/known-issues documents before upload.

Do not distribute this pair. The first Linux maintainer route exposed a rapid
brightness pulse on uppercase cockpit menu text that made the interface hard
to read. It also exposed two inaccurate assumptions in the tester guide:
Noctis IV OM has no game-audio playback path, and number keys entered while a
GOESnet terminal is selected belong to the terminal rather than the cockpit.

The correction keeps cockpit text at a stable brightness and tells testers to
deselect GOESnet, select Flight Control with `5`, and then start Vimana travel
with `7`. A fresh local Linux Release package passed verification and a focused
KDE Wayland/XWayland retest on an AMD Radeon RX 7800 XT: the text remained
steady and `5`, then `7`, started flight. That local result verifies the fix but
does not establish a new campaign candidate; new Linux and Windows CI archives
from the same corrected commit must be pinned first.

## Minimum closure evidence

W06 can move to `DONE` only when all of the following are recorded:

- at least two independent non-CI machines complete the core route on Linux and
  two on Windows;
- Linux evidence includes both an X11 session and a Wayland desktop using the
  supported XWayland path;
- the reports collectively include more than one graphics-vendor family rather
  than four near-identical environments;
- every report identifies an exact archive/checksum and includes startup
  diagnostics;
- every release-blocking report is reproduced or otherwise triaged with an
  owner and disposition;
- no open in-scope startup, crash/hang, data-loss/corruption, deterministic
  fixture, or core-control blocker remains;
- non-blocking differences are copied into the preview's known-issues document;
- the final results table records passes, failures, hardware/OS coverage, issue
  links, and the tested commit without publishing private player data.

Wine may provide additional Windows evidence but does not count as one of the
two independent Windows machines. Hosted CI does not count as a community
machine on either platform.

## Intake and triage

The GitHub `Community compatibility report` issue form requires the build
identity, environment, diagnostic output, checklist result, and reproduction
details. A maintainer should acknowledge each report, remove accidentally
shared sensitive material when possible, and classify it as:

- `blocker`: one of the release-blocking findings in the tester guide;
- `preview fix`: important and bounded enough to correct before the campaign
  closes;
- `known issue`: non-blocking, documented behavior accepted for the preview;
- `M8 follow-up`: valid stabilization work that does not invalidate the test;
- `not reproducible / more evidence needed`.

Corrections require a new CI artifact and checksum. Reports against the prior
artifact remain evidence for that build but do not prove the corrected build.
At least one reporter should repeat the affected route before closure.

## Launch gate

Do not publish invitations or call the packages community-test candidates until:

1. M7-W04 is complete and clean-start plus legacy-migration behavior use the
   final player-data locations.
2. M7-W05 is complete and its compatibility report and known-issues list ship
   in both packages.
3. Package verification confirms that this tester guide ships in both archives.
4. A maintainer performs the core route once on each final candidate or records
   why an assigned community run is the first manual route.

The replacement `7e764da` pair satisfies gates 1–3. Gate 4 still requires a
complete hands-on core route on each candidate. Until then, W06 remains
`IN PROGRESS` and recruitment is not open.
