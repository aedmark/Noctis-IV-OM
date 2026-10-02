# Noctis IV Plus feature ledger

Comparison: NIV+ at this repository's `5c46de9` versus Noctis IV LR at
`e1b0817da580e23062b3d2a64b7b6947bc0cb419`. Primary inventory source:
`source/docs/NIVPLUS_CHANGES.TXT` (Releases 1–2.3). LR's `README.md` says it
began from vanilla Noctis IV, not NIV+, and flags planet surface generation,
planetary object rendering, and GOESnet modules as incomplete. Static source
inspection is not a gameplay test; `NOT FOUND` below means the named NIV+
behavior has not been identified in LR's active source, not proof of absence.

`CORE` means a first-release parity requirement; `DEFERRED` is intentionally
outside that release. Scope does not imply implementation: the evidence column
describes the imported LR baseline, not completion in the new port.

| ID | NIV+ player-visible change | LR baseline evidence | Proposed scope | Acceptance evidence to capture |
| --- | --- | --- | --- | --- |
| P01 | `m` normal snapshot and `n` panoramic snapshot aliases | NIV+ handles the aliases in `source/NOCTIS.CPP` and `source/NOCTIS-1.CPP`; LR `modern/src/noctis.cpp` and `noctis-1.cpp` accept only `*` and `/` | CORE | Key-action fixture and output image |
| P02 | Panoramic snapshots do not corrupt game state or overwrite reserved files | NIV+ `snapshot()` uses `WIDE9997`–`WIDE9999`; LR `snapshot()` composes temporary `SNAP9997`–`SNAP9999` files in the normal namespace | CORE | Consecutive panorama and save/load regression |
| P03 | Roof speedup enabled, with `s` toggle while in Stardrifter | NIV+ `roofspeed` gates `sync_stop()` and is saved in `freeze()`; LR calls `sync_stop()` unconditionally and has no flag | CORE | Tick-distance comparison with toggle off/on |
| P04 | `b`/Delete raw snapshot, including from space | NIV+ space and surface input call `snapshot(..., 0)`; no equivalent binding exists in active LR input | CORE | Output image excludes target/name overlay |
| P05 | NICE-style numbered snapshot filenames and greater capacity | NIV+ `snapshot()` persists `lastSnapshot` and writes eight-digit names through 99,999,999; LR scans four-digit `SNAP%04d` names | CORE | Filename sequence/boundary fixture |
| P06 | Jetpack and jumping, including `c` directional-control cutoff | NIV+ `planetary_main()` has `jumping`/`jetpack` state and Space/`j`/`c` handling; active LR has none | CORE | Surface movement/input fixture |
| P07 | Three-mode surface mouselook | Pinned NIV+ cycles `option_mouseLook` with Down in `planetary_main()` (later than the release note's Up binding); active LR has no option state | CORE | Mode transitions and camera-motion fixture |
| P08 | Omega Drive | NIV+ `additional_consumes()` treats negative `charge` as infinite fuel and reports `OMEGA`; LR treats any nonzero charge as consumable and has no Omega state | CORE; ADR-0008 | Enablement, save flag, controls, and travel behavior |
| P09 | Moviemaker in space and on surface, with pause/settings | NIV+ has F3 menus, frame capture, decks, flash, and pause paths; active LR has no movie state | CORE; ADR-0013 | F3 state, safe deck/cadence output, pause, landing continuity, and ascent-cutoff fixtures |
| P10 | Radius in remote/local target information | NIV+ target-data paths print `RADIUS`/`CENTIDYAMS`; LR target-data paths omit radius | CORE | Target-window text fixture for known bodies |
| P11 | Surface status messages for mode changes | NIV+ `planetary_main()` renders `fcs_status_extended`; LR surface loop has no equivalent status render | CORE | Status text/timing fixture |
| P12 | `m` snapshot does not trigger or corrupt labeling | NIV+ guards the `m` binding with `!(labstar || labplanet)` before label input; LR has no `m` alias | CORE | Snapshot during star/planet label entry |
| P13 | Extended viewfield for objects | NIV+ `fragment()` halves object count only beyond depth 255; LR uses the vanilla depth-16 threshold | CORE; ADR-0008 | Canonical distance/culling frames |
| P14 | Bundled Help/GOESnet command access | NIV+ tracks `modules/HELP.com`; LR keeps translated modules under inactive `src/Old/` and its shell-driven GOESnet path is incomplete | CORE | Native equivalent command behavior, not DOS binary reuse |
| P15 | DL shows note count for selected planet rather than star | NIV+ `source/DL.CPP` queries `object_id`; LR's inactive `modern/src/Old/dl.cpp` contains that logic, but no DL module is built | CORE | Known labeled star/planet query |
| P16 | Epoc 6012 triad sinister calculation | NIV+ `wrouthud()` derives sinister with modulo 1e9; LR uses `fmod(secs, 1e9)` in the same display path but lacks boundary evidence | CORE | Epoc 6011→6012 display fixture |
| P17 | Zero-padded triad sinister/medius/dexter display | NIV+ uses `formatTriad()`/three-digit target formatting; LR pads the HUD components manually and uses `%03d` in target data | CORE | Epoc 6011→6012 display fixture |
| P18 | Cross-platform native startup preserves the outcome of the historic 2.2b Windows fix | The old DOS failure mechanism is not isolated; the required native outcome is now verified by hosted MSVC builds, native Windows startup, and Wine diagnostics/graphical smoke | CORE (native outcome) | Windows build plus diagnostic and graphical startup tests |
| P19 | Historical Tab antialias toggle from release 2.3 | Commit `8064249` removes the binding before pinned `5c46de9`; final NIV+ exposes visual settings through P25 instead | SUPERSEDED by P25 | Preserve history; test final pinned controls under P25 |
| P20 | F1 about/help with keyboard shortcuts | NIV+ `ShowAboutPage()` is reachable in space and on surfaces; active LR has no `about`/F1 path | CORE | F1 screen and key-map fixture |
| P21 | `s`/`p` do not activate ship functions while labeling | NIV+ guards both bindings with `!(labstar || labplanet)`; LR label input exists but those NIV+ ship bindings do not | CORE | Label text containing `s` and `p` |
| P22 | Updated starmap and guide content through 2023-10 | This repository pins `data/STARMAP.BIN` and `GUIDE.BIN`; LR deliberately excludes `data/` and has no initialization path | CORE; ADR-0007 | Data-version checksum, parser/query fixtures |
| P23 | Three-mode mouselook in space, unified with surface preference | Commits `141bf3b`, `54c9eb1`, and `ee12395`; final NIV+ space input cycles the shared option with Down; LR has no option | CORE | Space/surface mode persistence and camera-motion fixture |
| P24 | Raw panoramic snapshot from the surface | Commits `99751d1`/`64b289f`; final NIV+ uses `v` or `.` and suppresses overlays; LR panorama always follows its fixed show-data sequence | CORE | Three-frame panorama output with overlay exclusion |
| P25 | F2 visual-effects menu: HUD text, lens-flare mode, seamless border | Commit `8ba7c81`; final NIV+ implements the menu in both loops; active LR has no menu or option state | CORE | Menu transitions and before/after logical-framebuffer fixtures |

## Implementation-only and bundled-material notes

- Moving `SUPPORTS.NCT` out of the executable is an asset-loading change,
  already mirrored structurally by LR's `res/supports.nct`; verify hashes and
  runtime loading under M1/M5. It is not a separate player-visible feature.
- `defs.h`, Makefile, `compile.bat`, and other DOS build adjustments are not
  native gameplay requirements. Native build work belongs to M1.
- Release notes also mention an updated manual and bundled `HELP.com`. The
  native implementation needs help content/behavior, but reusing those files
  is subject to `PROVENANCE.md`.
- The changelog says NIV+ incorporates CE code and credits Mega, Neuzd,
  Bensel, The Reflection, Ees33, MopedSlug, Stargazer, Ireclan, Ryan J. Bury,
  and Mvgulik, while warning that some may not have contributions in NIV+.
  Attribution should follow actual migrated material; this list is not a
  substitute for a source/history provenance audit.
- The history audit compared vanilla `dda6bcde`, Release 1 `33187d8`, Release 2
  `5f97d3c`, Release 2.3 `62c27aa`, and pinned `5c46de9`. The Release 2 diff
  confirms the active Mega/Neuzd/SL features represented above. The named
  hopper-highlighting, exponential-pressure, and replacement-surface claims do
  not yield confirmed active differences in the pinned source: pressure and
  surface-generation lines blame to vanilla, and no active hopper marker was
  found. The changelog itself warns that named CE contributors may have no code
  in NIV+. No speculative ledger rows were added for those claims.
- Post-2.3 history was reviewed through pinned `5c46de9`. P23–P25 record the
  surviving player-visible additions. Build scripts, launcher behavior, manual
  link maintenance, and reverted intermediate visor/Tab experiments are not
  separate gameplay requirements.

Redistribution clearance is recorded in ADR-0007; earlier rights-review notes
are subject to that decision and do not block project work.

## M0-W06 review result

The original product-scope review made Omega Drive and extended viewfield core
under ADR-0008. ADR-0013 later restored Moviemaker to first-release scope. Source and
history review added P23–P25, reconciled the removed Tab binding, and documented
the unconfirmed CE claims without inventing features. Every row now names a
source path, commit, or bounded behavioral gap in LR. Runtime fixtures remain
the responsibility of the acceptance work named in the last column.

## M6-W01 bug-fix result

P02, P12, P16, P17, and P21 are now implemented with focused production
fixtures. P15 retains its completed M5-W05/W06 application evidence. P18's
native Windows outcome was subsequently completed in M7-W01 with hosted MSVC,
native Windows, and Wine evidence. Exact mappings and acceptance boundaries are
recorded in `NIVPLUS_BUG_FIXES.md` and `WINDOWS_MSVC.md`.

## M6-W02 controls result

P01, P03–P08, P23, and P24 are implemented. Normal/raw snapshot aliases,
surface panoramas, persisted eight-digit allocation, roof speed, shared
three-mode mouselook, jumping/jetpack control, and Omega Drive now run through
tested native boundaries. Exact control mappings and acceptance evidence are
recorded in `NIVPLUS_CONTROLS.md`.

## M6-W03 presentation result

P10, P11, P13, P20, and P25 are implemented. Target sheets show radius,
surface status accepts the extended Plus text, surface content keeps its full
population through depth 255, and F1/F2 expose contextual help plus persisted
HUD/flaring/border choices. Exact behavior and acceptance evidence are recorded
in `NIVPLUS_PRESENTATION.md`.

## M6-W04 content result

P22 is implemented as an explicit content package rather than an incidental
pair of repository files. `data/CONTENT_MANIFEST.json` pins the October 2023
digests, sizes, record geometry, counts, consolidated boundaries, and source
commit. The package verifier and focused fixture prove exact clean staging and
missing-only initialization that preserves player-modified runtime catalogs.
Native parser/query/application fixtures retain the semantic evidence. See
`NIVPLUS_CONTENT.md`.

## M6-W05 final disposition

| ID | Final state | Evidence or reason |
| --- | --- | --- |
| P01 | DONE | M6-W02 snapshot action and application fixtures |
| P02 | DONE | M6-W01 isolated panorama namespace and consecutive-capture regression |
| P03 | DONE | M6-W02 roof pacing and persisted-state coverage |
| P04 | DONE | M6-W02 normal/raw snapshot action coverage in both gameplay loops |
| P05 | DONE | M6-W02 eight-digit allocator boundary, collision, wrap, and persistence tests |
| P06 | DONE | M6-W02 jump, jetpack, capsule guard, and cutoff tests |
| P07 | DONE | M6-W02 three-mode surface camera transition tests |
| P08 | DONE | M6-W02 Omega enable/disable, recharge, consumption, save, and restart evidence |
| P09 | DONE | M6-W06 F3 controls, safe decks, deterministic cadence, production BMP output, pause/landing continuity, flash modes, and ascent cutoff evidence |
| P10 | DONE | M6-W03 remote/local target radius application fixture |
| P11 | DONE | M6-W03 extended surface status unit and frame evidence |
| P12 | DONE | M6-W01 label-entry action-suppression fixture |
| P13 | DONE | M6-W03 depth-255 population unit and surface frame/count evidence |
| P14 | DONE | M5-W04–W06 native HELP registry and production application workflows; M6-W03 contextual F1 help |
| P15 | DONE | M5-W05/W06 planet-specific DL note count through the native application |
| P16 | DONE | M6-W01 Epoc 6011→6012 rollover regression |
| P17 | DONE | M6-W01 shared zero-padded triad formatting regression |
| P18 | DONE | M7-W01 hosted MSVC suite, native Windows startup confirmation, and Wine diagnostics/three-frame graphical smoke |
| P19 | SUPERSEDED BY P25 | Removed before pinned NIV+; history is recorded and final controls are tested |
| P20 | DONE | M6-W03 F1 mapping plus space/surface contextual overlay fixture |
| P21 | DONE | M6-W01 label-entry action-suppression fixture |
| P22 | DONE | M6-W04 manifest/verifier, package-preservation test, and M5 semantic workflows |
| P23 | DONE | M6-W02 unified space/surface mode and persistence evidence |
| P24 | DONE | M6-W02 clean surface panorama action and overlay-exclusion behavior |
| P25 | DONE | M6-W03 F2 mapping, persisted menu transitions, and framebuffer evidence |

The review and M6-W06 now close every mandatory Linux feature, make no Windows
claim, and identify no other untracked mandatory omission. See
`NIVPLUS_SCOPE_REVIEW.md` and `NIVPLUS_MOVIEMAKER.md` for the final scope and
implementation evidence.
