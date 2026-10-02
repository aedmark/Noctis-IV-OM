# M6-W05 final Noctis IV Plus scope review

Status: complete. ADR-0013 reopened M6 for Moviemaker, which M6-W06 has now
completed before Windows verification.

## Result

Every one of the 25 player-visible ledger rows has a final disposition. All 24
current features are implemented with automated or reproducible evidence:

- P09 Moviemaker was restored to core scope by ADR-0013 and completed in M6-W06;
- P18 Windows-native startup was subsequently completed in M7-W01 with hosted
  MSVC, native-Windows confirmation, and Wine diagnostics/graphical evidence;
- P19's historical Tab antialias binding was removed by upstream NIV+ history
  and is superseded by the implemented P25 visual-effects menu.

The canonical row-by-row result is the final-disposition table in
`NIVPLUS_FEATURE_LEDGER.md`. W01 through W04 and the M5 GOESnet work provide
the linked implementation evidence. The current 38-test Linux suite passes
with Clang, GCC, and Clang ASan/UBSan.

## Reopened feature: Moviemaker

The user restored Moviemaker (P09) to the first-release feature set before
Windows-specific work. ADR-0013 supersedes only ADR-0008's P09 deferral; Omega
Drive and extended viewfield remain core under the original decision.

The source and bundled manual establish the native acceptance boundary:

- F3 opens the setup panel in space and on the surface without conflicting
  with F1 help or F2 visual settings;
- Ctrl-plus/minus selects decks 001–999, plus/minus selects a capture interval
  of 1–999 simulation frames, and `f` toggles the capture flash indicator;
- Enter starts or stops recording and `p` pauses or resumes it;
- each deck contains eight-digit BMP frames, while ordinary screenshots retain
  their independent `gallery/` namespace;
- recording continues across landing, while ascent retains the pinned
  100-gameplay-frame automatic cutoff needed to complete capsule recovery;
- recording rate is reported from deterministic simulation time, and pause
  does not add frames or elapsed recording time.

The native implementation keeps output non-destructive: an occupied deck
is reported and cannot be silently overwritten. Starting a new recording
resets its frame counter; stopping advances to the next deck candidate. The
game emits an image sequence only. ffmpeg remains an optional, user-invoked
post-processing tool and is not a runtime dependency.

## Other apparent omissions

- The removed Tab antialias toggle is not restored. It does not exist in the
  pinned final NIV+ state; P25 supplies the surviving visual settings.
- The DOS `HELP.com`, GOESnet executables, and catalog exchange/cleanup tools
  are not shipped. Their supported player behavior is native and tested;
  `CLEAN`, `INBOX`, and `OUTBOX` are intentionally retired.
- DOS compiler files, launcher/build scripts, and manual-link maintenance are
  implementation history rather than gameplay parity requirements.
- The updated manual, soundtrack, and unrelated historical binaries are not
  silently added to the native package. Their treatment remains governed by
  `PROVENANCE.md` and later release packaging work.
- Changelog references to hopper highlighting, exponential pressure, and a
  replacement surface were not confirmed as active pinned NIV+ deltas during
  M0-W06. This review does not invent requirements for unverified claims.
- Windows support is not an omitted feature or a Linux proxy result. MSVC CI,
  runtime startup, packaging, and P18 acceptance begin in M7-W01.

## M6 status

Bug fixes, controls, Omega Drive, extended viewfield, presentation settings,
contextual help, the October 2023 content package, and Moviemaker all have
recorded Linux evidence. M6 is complete. Windows validation is next and must
preserve the same behavior rather than redefine it.
