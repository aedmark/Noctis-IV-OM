# Initial compatibility fixtures

This document defines the first DOS-reference captures. F01A ADELPHE, F01B
JEHOVABOH, F01C MIRACLE, and F01D NEW FELYSIA have been recorded. F02A pins
FELYSIA's parent star, F02B records its repeated local-target properties with a
native match, and F02C records a replayable landing and surface sector with
matching native seeds. Inputs are pinned in `DOS_REFERENCE.md`.
Every capture must distinguish exact data/logic comparison from display appearance
and from time-dependent behavior. Do not modify tracked reference files in the
working checkout while capturing.

## Capture manifest

Each fixture directory will contain a `manifest.json` with at least:

```json
{
  "id": "F00",
  "reference_commit": "5c46de934ef69130b5b672f9cc3de703558364cb",
  "dosbox_x_version": "record from --version",
  "host_os_arch": "record at capture time",
  "config_sha256": "3da3ec703dad90829e854754586d735c8c97ab541d22e725c286b0f8cc151e2d",
  "input_sha256": {},
  "initial_state": "absent or named copy with hash",
  "steps": [],
  "output_sha256": {},
  "comparison_level": "EXACT | TOLERANT | STRUCTURAL | PERCEPTUAL",
  "notes": "known timing or rendering variability"
}
```

Capture commands, screenshots, raw save bytes, and extracted values alongside
that manifest when rights and repository-size policy permit. If captured files
cannot be committed, retain a hash and an exact regeneration procedure. Do
not include the Borland archive or unrelated DOS utilities in a fixture pack.

## First fixture set

| ID | Reference scenario | Input and output to pin | Comparison target |
| --- | --- | --- | --- |
| F00 | Clean startup and ship view | No `data/Current.BIN` in the **capture copy**; record first stable logical frame, visible HUD text, and files created on exit | Structural UI/state; perceptual frame |
| F01 | Galaxy/star identity | Pinned `data/STARMAP.BIN` label `MIRACLE` at zero-based byte 2732, preceding ID `e40006496f7d9f40`; label `NEW FELYSIA` at byte 36332, preceding ID `13562b7c5414f240`. Capture coordinates, class, and target text. | Exact raw IDs/classes/coordinates where represented as integers; tolerant displayed floating values |
| F02 | Planet properties and surface | Pinned `FELYSIA` planet label at zero-based byte 4364, preceding ID `a54de21d87b20f40`; F02A establishes its parent, F02B its properties, and F02C a replayed `LQ 001:060` surface | Exact serialized identity/state/seeds; structural planet and terrain class; perceptual palette/frame |
| F03 | Save and resume | On a fresh capture copy, save via normal exit after a fixed manual action sequence; hash `data/Current.BIN`, record size, then relaunch and compare restored state | Structural field-level state; byte-exact file only if clock/state determinism is established |
| F04 | Snapshot/rendering controls | At F01 target, capture normal, raw, and panoramic snapshots and Tab anti-aliasing states | Structural filenames/overlays; perceptual or exact indexed framebuffer where available |
| F05 | GOESnet/data lookup | Query known labeled star and planet, including note count when available, against pinned starmap/guide | Exact label and count text; no DOS executable reuse in native port |

The supplemental [F01A ADELPHE capture](fixtures/F01A_ADELPHE/manifest.json)
is the first accepted DOS galaxy/star reference. Two fresh DOSBox-X runs of
the pinned executable selected ADELPHE from the cartography list and saved
identical target coordinates, class, RGB, spin, and 32-bit radius. The native
galaxy/star test matches those fields exactly. The complete save files differ,
so their whole-file hashes are evidence of the runs, not byte-exact goldens.
The [F01B JEHOVABOH capture](fixtures/F01B_JEHOVABOH/manifest.json) repeats
the same method for a different star class (S08). Two fresh runs again agree
on all target fields, and the native result matches. The
[F01C MIRACLE capture](fixtures/F01C_MIRACLE/manifest.json) uses the DOS
GOESnet `PAR MIRACLE` and `ST MIRACLE` commands from a hashed console-pose
seed. Two direct runs agree on all target fields; a third exploratory run
agrees too. The native result matches. The
[F01D NEW FELYSIA capture](fixtures/F01D_NEW_FELYSIA/manifest.json) uses
`ST NEW FELYSIA` from that same console seed in two fresh runs. The separate
`PAR` query and both saved targets agree on coordinates; the native result
matches. This establishes the star. The separate
[F02A FELYSIA parent capture](fixtures/F02A_FELYSIA_PARENT/manifest.json)
shows that planet FELYSIA belongs to BALASTRACKONASTREYA, not NEW FELYSIA.
Two fresh `PAR FELYSIA` outputs are byte-identical; `PAR BALASTRACKONASTREYA`
returns the same coordinates, and the pinned starmap's P04 ID differs from
its parent-star ID by exactly 4.
The [F02B FELYSIA property capture](fixtures/F02B_FELYSIA_PROPERTIES/manifest.json)
starts two fresh DOS copies from the same generated P04-in-parent-system seed.
Both runs report FELYSIA P04, five major planets, a felisian radius of 0.0261,
a breathable environment, and the same revolution period. The native generator
matches those fields, including the period calculated with the legacy `4*pi/3`
mass factor.
The [F02C FELYSIA surface capture](fixtures/F02C_FELYSIA_SURFACE/manifest.json)
lands at `LQ 001:060` and replays the resulting 45-byte `SURFACE.BIN` from a
fresh root. Both entries preserve the landing coordinate, atlas tile, player
position, and HUD state. The native fixture matches the planet-wide surface
seed `952631` and local landing seed `60`. DOS shows a dark green vegetated
surface with bright plant forms; pixels and viewing angle remain time/input
dependent and are intentionally classified as structural/perceptual evidence.

| ADELPHE field | DOS (both direct runs) | Native | Result |
| --- | --- | --- | --- |
| Sector origin / rarity mask | Derived `(3300000, -4400000, -1100000) / 0` | Same input | Selection reaches DOS star |
| Star coordinates `(x, y, z)` | `(3352848, -4391963, -1106519)` | Same | Exact |
| Class / RGB / spin | `2 / (63, 63, 63) / 1` | Same | Exact |
| Radius | `0.45399999618530273` (`f32` bits `0x3ee872b0`) | Same bits | Exact |

| JEHOVABOH field | DOS (both direct runs) | Native | Result |
| --- | --- | --- | --- |
| Sector origin / rarity mask | Derived `(3900000, -4300000, -1100000) / 0` | Same input | Selection reaches DOS star |
| Star coordinates `(x, y, z)` | `(3897488, -4324932, -1025582)` | Same | Exact |
| Class / RGB / spin | `8 / (63, 32, 16) / 0` | Same | Exact |
| Radius | `5.185999870300293` (`f32` bits `0x40a5f3b6`) | Same bits | Exact |

| MIRACLE field | DOS (both direct runs) | Native | Result |
| --- | --- | --- | --- |
| Sector origin / rarity mask | Derived `(3900000, -5100000, -100000) / 0` | Same input | Selection reaches DOS star |
| Star coordinates `(x, y, z)` | `(3979984, -5143407, -98451)` | Same | Exact |
| Class / RGB / spin | `3 / (63, 30, 20) / 0` | Same | Exact |
| Radius | `31.233999252319336` (`f32` bits `0x41f9df3b`) | Same bits | Exact |

| NEW FELYSIA field | DOS (both direct runs) | Native | Result |
| --- | --- | --- | --- |
| Sector origin / rarity mask | Derived `(6600000, -4800000, -2400000) / 0` | Same input | Selection reaches DOS star |
| Star coordinates `(x, y, z)` | `(6555696, -4832326, -2337595)` | Same | Exact |
| Class / RGB / spin | `0 / (63, 58, 40) / 0` | Same | Exact |
| Radius | `6.913000106811523` (`f32` bits `0x40dd374c`) | Same bits | Exact |

The sector origins are inferred by matching the saved DOS coordinates against
the native sector function; DOS did not serialize them. The DOS cartography
list found ADELPHE and JEHOVABOH by their starmap IDs. DOS GOESnet independently
found MIRACLE and NEW FELYSIA, returned raw `PAR` and `ST` messages, and saved
both targets.

The native regression matrix complements those four DOS references. Its star
property rows cover every class from 0 through 11, including the distinct spin
branches for classes 2, 7, and 11. Its galaxy rows cover negative, zero, and
positive sector coordinates, both outcomes of the rarity filter, and the three
legacy empty-axis rejection points. These additional rows protect native
arithmetic and branch behavior; they are not represented as DOS observations.
The displayed Y coordinate is positive by convention; the internal saved Y is
negative. These coordinates come from DOS's own lookup, not native output.

The raw eight-byte IDs above were extracted from the pinned starmap record
bytes immediately before each 24-byte label. MIRACLE and NEW FELYSIA have
DOS-reachable target captures. `PAR FELYSIA` reaches the planet's starmap entry
and establishes its parent-star relationship. F02B and F02C add its physical
properties and first visitable surface reference. Additional planet classes
remain future coverage.

## Reproducible capture procedure

1. Copy only the needed tracked game/reference paths into a disposable capture
   directory. Verify their SHA-256 values against `DOS_REFERENCE.md` **before**
   launch. Never run fixture capture against the main checkout because the
   DOS program writes `data/Current.BIN` and may modify map/guide files.
2. Record DOSBox-X version, host OS/architecture, display scaling, full
   `dosbox.conf` hash, and whether starting `Current.BIN` is absent or a
   separately hashed fixture input. Preserve a clean pre-run file manifest.
3. Launch from the capture directory with the same sequence as the existing
   scripts: mount its root as `N:`, switch to `N:`, `cd modules`, run
   `NOCTIS.EXE` using the pinned `dosbox.conf`.
4. Follow exact numbered interactions. Capture screenshots at stable states,
   not after assumed wall-clock delays. Record both the DOSBox output image
   and any in-game BMP. For procedural tests, record displayed values and raw
   object IDs; screenshots alone do not establish galaxy compatibility.
5. Exit normally, hash all outputs, compare the pre/post file manifests, and
   note any writes. A fresh capture directory is required for each fixture or
   an explicitly chained fixture set.
6. Repeat once in a second fresh copy. If bytes differ, classify whether the
   cause is clock, uninitialized state, emulator timing (`cycles=max`), or
   rendering before declaring a byte-exact target. Never bless unstable bytes
   as golden fixtures.

## Open setup details

- The locally installed DOSBox-X reports `2026.08.31 SDL1`; the F01A launch
  and capture path is validated. `cycles=max` in the pinned configuration can
  make timing observations host-dependent.
- No live `data/Current.BIN` is checked in. F01C contains a fixture-specific
  `seed-current.bin`; F03 will create its own seed save in an isolated capture
  copy and record its size/hash.
- Manual input sequences for MIRACLE, NEW FELYSIA, FELYSIA's parent lookup,
  and the F02C landing are captured. F02 now has repeated target data plus a
  replayed surface state and palette observation. F05 has star
  and planet `PAR` lookup output, not its
  planned note-count coverage. Do not infer gameplay parity from a starmap
  record inspection alone.
- F01A/F01B screenshots are included as evidence. F01C/F01D/F02A include raw
  GOESnet files and share a deterministic seed. F01D also includes one
  replayable engaged-target save; it is not a whole-file golden. Other
  captured `CURRENT.BIN` samples remain in disposable capture directories;
  hashes and regeneration steps are recorded in the manifests.
