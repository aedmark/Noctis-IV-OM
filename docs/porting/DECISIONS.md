# Decision Log

This log records choices that constrain future work. Use sequential identifiers
(`ADR-0001`, `ADR-0002`, ...). A decision is immutable after acceptance; if it
changes, add a new record that marks the old one `SUPERSEDED`.

## Index

| ID | Decision | Status | Date |
| --- | --- | --- | --- |
| ADR-0001 | Continue from Noctis IV LR in modern C++ | ACCEPTED | 2026-09-22 |
| ADR-0002 | Documentation and session continuity format | ACCEPTED | 2026-09-22 |
| ADR-0003 | Import LR as an unsquashed subtree under `modern/` | SUPERSEDED by ADR-0004 | 2026-09-25 |
| ADR-0004 | Separate local integration from public distribution review | SUPERSEDED by ADR-0005 | 2026-09-25 |
| ADR-0005 | Accept LR license compliance as a project assumption | ACCEPTED | 2026-09-25 |
| ADR-0006 | Use C++20 with pinned LR dependencies and two Linux compilers | ACCEPTED | 2026-09-25 |
| ADR-0007 | Record project redistribution clearance and conditions | ACCEPTED | 2026-09-26 |
| ADR-0008 | Set first-release scope for three optional NIV+ features | SUPERSEDED in part by ADR-0013 | 2026-09-26 |
| ADR-0009 | Defer Windows verification until the cross-platform preview | ACCEPTED | 2026-09-27 |
| ADR-0010 | Use fixed 55 ms simulation ticks independent of presentation timing | ACCEPTED | 2026-09-27 |
| ADR-0011 | Make versioned native state authoritative before legacy migration | ACCEPTED | 2026-09-29 |
| ADR-0012 | Import only identified legacy save layouts through temporary state | ACCEPTED | 2026-09-29 |
| ADR-0013 | Restore Moviemaker to first-release Plus scope | ACCEPTED | 2026-09-30 |

## ADR-0001 — Continue from Noctis IV LR in modern C++

Status: ACCEPTED  
Date: 2026-09-22

### Context

The Noctis IV Plus codebase is tied to 16-bit Borland C++, DOS services, VGA
hardware, segmented memory, and extensive x86 assembly. Noctis IV LR has already
reimplemented much of that foundation using modern C++, CMake, Raylib, and GLM.

### Decision

Use Noctis IV LR as the technical starting point and migrate Noctis IV Plus
behavior into it. Do not attempt a line-by-line modernization of the DOS source.
The exact upstream commit and repository integration mechanism remain M0 work.

### Consequences

- The project inherits LR's implemented systems and known divergences.
- NIV+ changes require an explicit feature ledger and compatibility fixtures.
- LR provenance, permission, and attribution must be verified before release.
- Existing LR dependencies remain the default until evidence supports changing
  them.

## ADR-0002 — Documentation and session continuity format

Status: ACCEPTED  
Date: 2026-09-22

### Context

The port will span many sessions and contains technical, compatibility, and
provenance dependencies. A chronological diary would not reliably communicate
the current state.

### Decision

Maintain four complementary documents:

- `BLUEPRINT.md` for durable intent and architecture;
- `ROADMAP.md` for capability status and exit criteria;
- `SESSION_HANDOFF.md` for the single current resumable state;
- `DECISIONS.md` for consequential choices and their rationale.

The handoff is replaced at the end of every work session. Git history provides
the archive. Roadmap status changes require evidence or an explicit blocker.

### Consequences

- A new session has one obvious starting point.
- Repeated narrative is minimized.
- End-of-session documentation is part of the definition of done.

## ADR-0003 — Import LR as an unsquashed subtree under `modern/`

Status: SUPERSEDED by ADR-0004

Date: 2026-09-25

### Context

This repository is the existing Noctis IV Plus home. Its DOS source, data,
manual, and Git history are needed for comparison throughout the port. Noctis
IV LR has a separate 283-commit history and overlapping root names such as
`README.md`, `.gitignore`, and `docs`/`doc`. M0-W02 audited LR commit
`e1b0817da580e23062b3d2a64b7b6947bc0cb419` as a buildable candidate.

We considered three layouts:

| Option | Benefit | Cost |
| --- | --- | --- |
| Separate LR fork/repository | Clean LR root and easy upstream comparison | Port planning, NIV+ history, and reference files become split across repositories; this project's handoff would not describe the implementation tree |
| Merge unrelated histories at the repository root | One checkout and both ancestries | Root path collisions require a large move or conflict resolution at the initial merge; DOS reference and active port become difficult to distinguish |
| Unsquashed LR subtree at `modern/` | One checkout, distinct legacy and native trees, both ancestries retained, later upstream sync possible | Native build is nested; inherited CI under `modern/.github` does not run as root CI; data paths and packaging need explicit handling |

### Decision

Keep this repository as the project home. At M1-W01, import the audited LR
commit into `modern/` with `git subtree add --prefix=modern` **without**
`--squash`. Perform the import on a `codex/modern-port` branch only after
M0-W04 records provenance **and** the permission gate in `PROVENANCE.md` is
resolved. Keep the original `source/`, `modules/`,
`data/`, and `manual/` trees at their current paths as comparison material.

The LR commit above is the import target for now. A newer commit requires a
fresh M0-W02 audit and an amendment/superseding decision before import. Keep a
read-only `noctis-lr` remote for later upstream review; do not automatically
pull new commits into the subtree.

### Verification

A throwaway clone of this repository at `5c46de934ef69130b5b672f9cc3de703558364cb`
successfully imported the LR commit under `modern/` using an unsquashed
subtree. The resulting merge commit had both repository heads as parents;
`git subtree split --prefix=modern` returned
`e1b0817da580e23062b3d2a64b7b6947bc0cb419`. `git blame` on
`modern/src/brtl.cpp` resolved original LR author commits. No subtree was
added to the working project by this verification.

### Consequences

- The two source histories remain available in one clone. Import commits must
  preserve LR license files, contributor records, and commit ancestry.
- The native app's build root is `modern/`; project-level CI must live at the
  repository root rather than relying on LR's nested workflow.
- The root `data/` tree is legacy reference data. Native writable data paths
  and any intentional asset reuse must be designed separately in M1/M5.
- Upstream synchronization should use reviewed, unsquashed subtree pulls on a
  dedicated branch, with compatibility tests before integration.
- The implementation step remains gated by evidence of modification and
  downstream rights (see `PROVENANCE.md`). Completing the provenance inventory
  does not itself clear that gate. This ADR chooses a technical layout; it does
  not resolve distribution rights.

## ADR-0004 — Separate local integration from public distribution review

Status: SUPERSEDED by ADR-0005

Date: 2026-09-25

Supersedes: ADR-0003's permission timing only; retains its unsquashed
`modern/` subtree layout and pinned LR commit.

### Context

The project user identified Alessandro Ghignola's own 80.style site. Its
[profile](https://80.style/#/about/hsp) identifies him as developer/maintainer;
its [Noctis IV collection](https://80.style/#/hsp/noctis_iv) features Noctis IV
Plus and CE and enthusiastically showcases Owen Arthur's TypeScript NICE TRY
reimplementation. This is direct evidence of creator support for fan ports,
though neither page supplies blanket licensing terms. WPL section 4(b) and
third-party contributor/asset rights still matter for a public distribution.
Keeping the entire engineering effort blocked while seeking a general
statement is disproportionate to this evidence and the user's intent.

### Decision

Proceed with **local** LR integration and C++ development, retaining
Alessandro's and LR/NIV+ contributors' credits, source notices, and full Git
history. Do not push the imported branch or publish a native package until the
combined project's modification/distribution terms and included assets have
been reviewed. Record the outcome in `PROVENANCE.md` and revisit the gate at
M8-W04. The local import remains M1-W01 on `codex/modern-port` at the exact
audited LR commit.

### Consequences

- M1-W01 is no longer blocked by the search for a general fan-port statement.
- Local build, tests, and compatibility work can advance while provenance
  review continues.
- Creator endorsement is treated as meaningful precedent, not as a license for
  every LR contribution or third-party asset.
- No remote push or release is implied by this decision.

## ADR-0005 — Accept LR license compliance as a project assumption

Status: SUPERSEDED in part by ADR-0013

Date: 2026-09-25

Supersedes: ADR-0004's special LR permission review. Retains the unsquashed
`modern/` subtree integration strategy and attribution requirements.

### Context

The project user explicitly directed us to assume that material checked into
Noctis IV LR abides by its stated license. LR includes the WTOF license,
credits its contributors, and identifies conditions received from Alessandro
in its README. Repeating an independent investigation of the underlying LR
permission chain would halt the requested engineering work without adding
useful technical evidence. This is a project assumption, not a legal finding.

### Decision

For this port, treat the pinned LR repository content as license-compliant and
available for the planned subtree integration and continued C++ work. Preserve
its license text, contributor credits, and unsquashed history, and prominently
credit Alessandro Ghignola. Do not impose an LR-specific permission checkpoint
on M1-W01 or on subsequent technical work. Continue to review licensing and
attribution for *new* material brought in from outside LR, including NIV+
assets, third-party archives, and future dependencies. Packaging review at
M8-W04 checks notices and package contents; it does not reopen the assumed LR
grant chain absent contrary evidence.

### Consequences

- The team can integrate LR and build on it without repeated permission debate.
- `PROVENANCE.md` distinguishes user-directed assumptions from source-verified
  facts; neither is silently substituted for the other.
- This decision does not itself authorize an external push or release action;
  those remain separate workflow steps.

## ADR-0006 — Use C++20 with pinned LR dependencies and two Linux compilers

Status: ACCEPTED

Date: 2026-09-25

### Context

The audited LR commit builds with Clang 22.1.8 on Linux. A temporary copy
without its hard-coded compiler override also built with GCC 16.2.1. LR's
current CMake does not declare a C++ language version and fetches Raylib and
GLM by movable tags. Modern-port work needs a declared language level,
reproducible dependency identity, and compiler diversity before behavior is
treated as stable. Windows remains a target but has not been verified here.

### Decision

- Target C++20, without compiler-specific language extensions. M1-W02 will
  express this on the executable target with `cxx_std_20`, disable extensions,
  and remove LR's `/usr/bin/clang`/`clang++` override. Do not turn existing
  first-party warnings into build failures until they are triaged.
- Use CMake 3.21 or newer for the planned preset workflow. The observed host
  has CMake 4.4.3; the minimum is a project requirement to verify in M1,
  not a claim of having tested every intermediate CMake release.
- Keep Raylib and GLM initially. Pin Raylib 6.0 to commit
  `dbc56a87da87d973a9c5baa4e7438a9d20121d28` and GLM 1.0.3 to commit
  `8d1fd52e5ab5590e2c81768ace50c72bae28f2ed`, rather than relying only
  on tag names. Keep the LR CPM 0.43.1 bootstrap with its existing SHA-256
  `1c40fc102ce9625d7de7eb14f541cab30cc3138dca627f0b0ec40293ce6c2934`.
- Use Clang as the initial Linux build/sanitizer compiler and GCC as an
  independent Linux compile/test lane. The versions above are observed
  baselines, not permanent global minimums. MSVC/Windows support remains an
  explicit M7 verification target; do not claim it works from Linux results.

### Verification and limits

At the pinned LR commit, all four first-party translation units passed
`-std=c++20 -fsyntax-only` with Clang 22.1.8 and GCC 16.2.1 using the fetched
Raylib 6.0 and GLM 1.0.3 headers. Clang emitted the same six first-party
warnings noted in `LR_BASELINE_AUDIT.md`; GCC's syntax check emitted none.
This is language-compatibility evidence, not a full C++20 linked build or a
runtime test. M1-W02/M1-W03 must perform those checks with normalized CMake.

### Consequences

- C++20 is now a contract rather than an implicit compiler default.
- M1-W02 owns compiler selection, presets, and strict-standard expression.
- M1-W03 owns exact dependency pinning and notice handling in the imported
  build. Local caches may accelerate builds but may not define dependency
  identity.
- Earlier UBSan findings remain active; the toolchain decision does not mark
  them fixed.

## ADR-0007 — Record project redistribution clearance and conditions

Status: ACCEPTED
Date: 2026-09-26

Supersedes: unresolved project redistribution gates in `PROVENANCE.md` and
related feature-ledger notes; retains ADR-0005 and existing notices.

The project user explicitly confirmed: "We are cleared for redistibution as
long as we keep it open and credit the original author".

Treat redistribution of this project as cleared on those conditions: keep the
project open source and prominently credit Alessandro Ghignola. Preserve all
existing license texts and contributor notices. This records the user's
confirmation; it does not invent new license terms or a separate permission
instrument. Review newly introduced third-party material under its own terms.
There is no remaining project-wide permission checkpoint for continued port
work or redistribution. Release publication still requires a release task.

## ADR-0008 — Set first-release scope for three optional NIV+ features

Status: SUPERSEDED in part by ADR-0013
Date: 2026-09-26
Supersedes: none

### Context

M0-W06 left Omega Drive, Moviemaker, and extended object viewfield marked for
product review because the NIV+ release notes establish their presence but do
not determine whether each belongs in first-release parity.

### Decision

Treat Omega Drive (P08) and extended object viewfield (P13) as core
first-release NIV+ parity. Defer Moviemaker (P09) from the first release.

### Consequences

- M6 planning and acceptance coverage must include Omega Drive and extended
  viewfield.
- Moviemaker is not a first-release requirement. Track the omission explicitly
  and reconsider it through M6-W05 rather than allowing it to disappear from
  the feature ledger.
- This decision resolves the three outstanding M0-W06 product-scope choices;
  the source/history and LR-evidence reviews remain open.

## ADR-0009 — Defer Windows verification until the cross-platform preview

Status: ACCEPTED
Date: 2026-09-27
Supersedes: none

### Context

M1's Linux build, test, sanitizer, startup, and contributor workflows are
verified. Windows remains a release target, but validating it now would split
effort before the Linux implementation and compatibility behavior are stable.

### Decision

Close M1 against its documented Linux development scope. Defer Windows preset,
MSVC, runtime, documentation, CI, and packaging verification to M7, beginning
with M7-W01. Do not claim Windows support before that work is complete.

### Consequences

- M1-W02 and M1-W07 can close on their verified Linux evidence.
- Linux is the only supported development platform until M7 Windows work lands.
- M7 retains the existing cross-platform exit criteria; this is a scheduling
  decision, not a reduction in the intended release platforms.

## ADR-0010 — Use fixed 55 ms simulation ticks independent of presentation timing

Status: ACCEPTED
Date: 2026-09-27
Supersedes: none

### Context

The inherited loop derived fractional universe time from the number of frames
rendered during the current wall-clock second. It also used the C CPU clock for
simulation seeds and visual phases while independently sleeping after buffer
swaps. Machine and rendering performance could therefore change simulation
state. The DOS-compatible cadence already has an inherited 55 ms contract.

### Decision

Synchronize the universe epoch to wall time only at startup/save restoration.
Advance ordinary simulation with an integer-counted fixed 55 ms tick. Use that
tick for deterministic state, animation phase, and procedural seeds. Use
`steady_clock` only for presentation pacing and real-time input windows.

### Consequences

- Equal tick and input sequences produce equal simulation time regardless of
  measured rendering throughput.
- Offline elapsed time remains tied to the existing universe epoch behavior.
- M3-W06 must test the eventual slow-frame catch-up/skip policy through a
  scripted journey; this decision does not prescribe that policy.
- The fixed cadence may be changed only through a superseding compatibility
  decision and updated fixtures.

## ADR-0011 — Make versioned native state authoritative before legacy migration

Status: ACCEPTED
Date: 2026-09-29
Supersedes: none

### Context

The inherited `current.bin` writes individual globals without a header,
version, checksum, complete-read checks, or an upgrade path. Old GOESnet
executables still consume that file, while existing fixture saves must remain
importable. Treating the raw layout as the new port's format would make later
schema evolution and safe validation impossible.

### Decision

Use `current.niv` as the authoritative native situation file. Encode every v1
field explicitly in little-endian order inside a versioned, sized, checksummed
envelope and replace the file through a temporary. Prefer native state whenever
it exists and refuse invalid or unknown native files. Fall back to
`current.bin` only when native state is absent, until M5-W02 supplies its
bounded importer. Continue emitting `current.bin` solely for inherited
GOESnet interoperability until M5-W05.

### Consequences

- Runtime structure layout and compiler ABI are no longer a native disk format.
- Future schemas require a new version and explicit conversion path.
- A corrupt native save cannot be hidden by a stale legacy file.
- Legacy situation and surface checkpoints remain separate, explicitly bounded
  follow-up work rather than being blessed as native formats.

## ADR-0012 — Import only identified legacy save layouts through temporary state

Status: ACCEPTED
Date: 2026-09-29
Supersedes: none

### Context

Vanilla, NIV+, and LR-derived saves are unversioned byte sequences whose sizes
changed as fields were appended. The inherited reader accepted short reads and
applied data directly to globals, so truncation could mix file contents with
whatever values happened to be in memory. NIV+ history also contains a
transitional 382-byte layout that is not a prefix of the pinned format.

### Decision

Recognize only exact layouts evidenced by checked-in documentation, source
history, or tracked fixtures. Read no more than the largest supported size,
decode little-endian values into temporary fixed-width state, validate known
selectors, strings, numbers, and preferences, and apply nothing unless the
whole import succeeds. Migrate accepted situation and surface state to native
v1 without deleting the source file. Never use legacy state as fallback when a
native file exists but is invalid.

### Consequences

- Truncated, oversized, unknown, and structurally invalid files fail without
  partially changing runtime state.
- The transitional 382-byte HUD layout has an explicit conversion rather than
  being mistaken for the pinned 381-byte prefix.
- Historical formats outside the evidenced table are rejected and can be added
  later only with provenance and a focused fixture.
- Exhaustive corruption and multi-version upgrade testing remains M5-W07.

## ADR-0013 — Restore Moviemaker to first-release Plus scope

Status: ACCEPTED
Date: 2026-09-30
Supersedes: the P09 deferral in ADR-0008

### Context

ADR-0008 made Omega Drive and extended viewfield core while deferring
Moviemaker. After the other M6 work was complete, the user explicitly reopened
Moviemaker before Windows-specific work. The pinned source and bundled manual
show one coherent feature shared by space and surface play: an F3 setup panel,
numbered frame decks, adjustable capture cadence, flash indication,
start/stop, pause/resume, landing continuity, and a bounded ascent cutoff.

### Decision

Make Moviemaker (P09) a core first-release NIV+ parity requirement and reopen
M6 for M6-W06. Implement native image-sequence capture only; external video
encoding remains an optional user workflow and the game will not invoke
ffmpeg. Preserve the final pinned controls and observable lifecycle while using
portable paths, bounded state, and non-destructive output allocation.

### Consequences

- M6 is not complete until M6-W06 has focused state/output tests and production
  space/surface workflow evidence.
- F3, Ctrl-plus/minus, plus/minus, `f`, Enter, and `p` require semantic native
  routing without regressing F1/F2, labels, lighting, or ordinary snapshots.
- Frame output belongs under numbered `movies/001`–`999` decks with eight-digit
  BMP names. Existing decks must not be silently overwritten.
- Capture cadence follows simulation frames and remains deterministic under the
  fixed-tick model; pause time does not manufacture output frames.
- ADR-0008 remains authoritative for Omega Drive and extended viewfield; only
  its Moviemaker deferral is superseded.

## ADR-0014 — Separate installed assets from OS-native player profiles

Status: ACCEPTED
Date: 2026-09-30
Supersedes: the colocated mutable package layout established by M7-W02/W03

### Context

The first portable preview packages kept resources, catalog seeds, saves,
screenshots, and Moviemaker decks beside the executable. That made the launch
directory part of the application contract, required a writable installation,
mixed immutable distribution content with player state, and encouraged saves
inside disposable build trees. Linux and Windows also provide different
standard locations for per-user data and configuration.

### Decision

Keep runtime resources and verified default catalogs beside the executable, but
place mutable player data in the platform's user-data area and configuration in
its user-configuration area. Resolve Linux roots with XDG variables and HOME
fallbacks; resolve Windows roots through Known Folder APIs. Provide an explicit
profile override and a non-destructive importer for old portable roots. Copy
only missing regular files, skip links, preserve collisions, and never delete
the source. Keep diagnostics read-only and report all resolved paths.

### Consequences

- Packages and build directories are disposable and need not be writable at
  runtime; immutable catalog seeds live under `defaults/`.
- Existing colocated profiles remain importable in place or through
  `--migrate-from`, while destination data always wins a collision.
- Tests and package checks must select isolated profiles rather than relying on
  their working directories.
- Existing preferences remain in the versioned save until a separately
  designed standalone configuration format is justified.
- The W02/W03 CI runs remain evidence for producing packages, but a new Windows
  package run is required to accept this revised cross-platform layout.

## Decision record template

```markdown
## ADR-NNNN — Imperative decision title

Status: PROPOSED | ACCEPTED | SUPERSEDED
Date: YYYY-MM-DD
Supersedes: ADR-NNNN or none

### Context

What forces the choice? Include constraints and viable alternatives.

### Decision

State the choice directly.

### Consequences

- Benefits and enabled work.
- Costs, risks, and follow-up obligations.
```
