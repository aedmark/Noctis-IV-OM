# Port provenance and permission register

Status: project redistribution cleared by the user's explicit confirmation on
2026-09-26, conditional on keeping the project open source and crediting the
original author, Alessandro Ghignola (ADR-0007). Preserve existing license and
contributor notices. Historical unknowns below describe the earlier inventory;
they do not impose a continuing project redistribution gate. Newly introduced
third-party material still requires its own provenance review.

**Project assumption (ADR-0005):** per the user's explicit direction, material
already checked into the pinned Noctis IV LR repository is treated as abiding
by its stated license. Do not reopen LR's underlying grant chain as a routine
engineering blocker. This assumption does not automatically cover material
newly introduced from NIV+ or elsewhere.

## Evidence standard

`Documented` means the identified file states a license or credit. `Claimed`
means a project maintainer reports a permission without the underlying grant
being present here. `Unknown` means the checked sources do not establish the
necessary scope. A Git commit or a publicly downloadable file establishes
provenance, not permission. Keep the original wording of licenses intact.

## Source and asset register

| Material | Origin and evidence | Permission evidence | Planned treatment |
| --- | --- | --- | --- |
| Original Noctis IV code and game assets in `source/` and `modules/` | Alessandro Ghignola is credited in `source/docs/WPL.HTM`; original build notes are in `source/docs/compiling_noctis.txt`. His own 80.style Noctis collection showcases fan modifications and a TypeScript reimplementation. | **Documented:** WTOF Public License (WPL) permits qualifying unmodified redistribution but section 4(b) reserves source modification to copyright holders or their express designees. **Documented practice:** creator endorsement of named ports. **Unknown:** terms for this project's public distribution. | Preserve WPL and prominent Alessandro credit. Continue local port work; review release terms separately. |
| Noctis IV LR C++ translation and `res/` | [`dgcole/noctis-iv-lr`](https://github.com/dgcole/noctis-iv-lr) at `e1b0817da580e23062b3d2a64b7b6947bc0cb419`; `CONTRIBUTORS.md` credits Dyllan Cole and Bryce Clark; history also records Joris van de Donk. | **Documented:** LR includes WPL in `LICENSE.md`; its README states Alex's original-gameplay condition. **Project assumption (ADR-0005):** all checked-in LR material abides by that license; the underlying grant is not independently re-investigated. | Integrate and continue the C++ work with `LICENSE.md`, `CONTRIBUTORS.md`, prominent Alessandro credit, and unsquashed history intact. |
| Noctis IV Plus modifications | This repository's history credits Joris van de Donk and Ella Jameson among other commit authors; `README.md` describes NIV+ as a modification. `source/docs/NIVPLUS_CHANGES.TXT` credits Mega and Neuzd as primary workers and names CE contributors, with a caveat that not all their work appears in NIV+. WPL copy is under `source/docs/`. | **Unknown:** the precise grant covering original-source modification and downstream maintenance of this C++ effort; each contributor's terms for new material also need confirmation. A first-person account of earlier CE permission is relevant evidence, not by itself a grant to all later maintainers. | Track NIV+ differences in M0-W06; migrate material with confirmed permission and preserve relevant credits. |
| Shared maps and support data | LR `res/supports.nct`, `res/globes.map`, and `res/offsets.map` are byte-identical to this repository's corresponding `source/` files (see `LR_BASELINE_AUDIT.md`). LR history shows a 2019 resource-file commit. | **Unknown:** matching hashes establish common bytes, not who may republish or alter them. | Classify with original game assets until permission is resolved. |
| Starmap and guide data | `data/STARMAP.BIN` and `data/GUIDE.BIN` were updated in Joris van de Donk commit `3759857` (2023-10 Department of Astrocartography data); exact package identity is recorded in `data/CONTENT_MANIFEST.json`. | **Project clearance:** ADR-0007 covers checked-in project material under its open-source and credit conditions; individual community-entry authorship is not enumerated. | Bundle the exact verified archive, preserve project/contributor credit, and keep mutable player catalogs separate from immutable release seeds. |
| Manual, graphics, soundtrack | `manual/noctis_iv_manual_old.html` credits Ryan J. Bury for manual, non-screenshot graphics, and soundtrack. WPL names him for the English manual and MIDI. The repository also has a later MP3 added via a manual update. | **Documented:** author credit. **Unknown:** whether the MP3 and altered HTML are covered by the same permission as the original manual/MIDI, and whether adaptation/repackaging is authorized. | Do not automatically bundle or edit these in the native package. Preserve credit and verify scope first. **Maintainer decision (2026-10-02):** the updated manual HTML is published on the project website as `manual/noctis_iv_manual.html` with its text and credits unchanged; only its image paths were repointed to the identical files in `doc/img`. The soundtrack MP3/MIDI is not redistributed, and its autoplay element was removed. The native packages still exclude the manual. |
| DOS executables and utilities | `modules/` contains `NOCTIS.EXE` and helper executables; `source/` contains `MAPS.EXE` and `POLYVERT.EXE`. Attribution is mixed or not explicit per binary. | **Unknown:** per-binary ownership and redistribution basis. | Use locally as compatibility references only; do not ship in the native package without review. |
| Borland C++ 3.1 archive | Tracked `BCPP31.ZIP` (about 15 MB), added by commit `5c46de9`; root README links to a Borland archive for the DOS build. | **Unknown:** no Borland redistribution license found in this repository. WPL is not evidence of rights to third-party compiler software. | Exclude from native import, dependencies, and release artifacts. Review whether it should remain tracked separately; do not silently remove user data. |
| Raylib, GLM, CPM | LR CMake fetches Raylib and GLM through CPM. The audited build's fetched Raylib `LICENSE` is zlib-style; GLM `copying.txt` offers Happy Bunny or MIT; CPM's own notice identifies MIT. | **Documented:** separate upstream dependency notices, subject to their exact versions and packaging obligations. | Pin revisions in M0-W08/M1-W03 and include applicable notices in distributable packages. These licenses do not clear Noctis rights. |

## Primary terms and permission gap

The included WPL is not a conventional open-source modification grant. Its
section 4(b) explicitly requires express authorization from the relevant
copyright holder(s) to change licensed source. `compiling_noctis.txt` says
private experiments are possible, while distribution of changed versions
needs authorization. Section 4 also imposes conditions on redistribution,
including no charge for WPL-covered material and preserving a license reference.
The exact license text, rather than this summary, controls.

The user-provided [80.style profile](https://80.style/#/about/hsp) identifies
Alessandro Ghignola as the site's original developer and current maintainer.
It calls his hobby projects "abandonware" in the sense that he no longer
develops modern versions himself; that word is a description, **not a license
or waiver**. His [Noctis IV collection](https://80.style/#/hsp/noctis_iv)
links Noctis IV Plus, Noctis IV CE, and Owen Arthur's [TypeScript NICE TRY
reimplementation](https://80.style/#/hsp/noctis_iv/typescript_implementation_Jnice_tryBK).
The latter receives warm, explicit editorial praise. This is primary-source
evidence that Alessandro welcomes at least that fan port. The page does not
state a general license or an exact "credit only" condition.

LR's README reports that Alex allowed its release if original gameplay was
preserved. The underlying correspondence was not found in either checked
repository, but ADR-0005 directs us to assume LR's checked-in content abides
by its license and proceed. Joris's [first-person Noctis history](https://mooses.nl/noctis-iv/)
also describes a 2004 permission for his/Shadowlord's modification.

On 2026-09-25, the current project user reported that Alessandro welcomes fans
creating their own ports provided he is credited. The 80.style pages corroborate
the welcoming practice and site authorship, though not a blanket credit-only
grant. Record any more explicit statement if it turns up; do not stall local
engineering while searching for it.

## Current distribution conditions (ADR-0007)

1. Keep the project open source and prominently credit Alessandro Ghignola.
2. Preserve LR, NIV+, and other contributor credits, license texts, and history.
3. Review newly introduced third-party assets and code under their own terms;
   do not assume project clearance grants rights to unrelated bundled tools.
4. At packaging time verify notices, source availability, and package contents.
   Do not reopen the resolved project-wide permission checkpoint.

The clearance is recorded from the user's confirmation, not external outreach
or a newly obtained license document.
