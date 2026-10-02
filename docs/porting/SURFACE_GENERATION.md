# M4-W01/W02 Surface Generation Characterization

M4-W01 establishes the first complete native surface-generation fixture for
FELYSIA at the DOS-confirmed landing coordinate `LQ 001:060`. The fixture runs
the production orbital texture, environment, terrain, ground-texture, ruin, and
object-placement paths without opening a host window.

## Data model and seed flow

The active generator divides a planet into 360 longitude positions by 120
latitude positions. A selected sector produces three principal buffers:

| Artifact | Logical shape | Role |
| --- | --- | --- |
| Orbital/ground texture (`txtr`) | 256×256 bytes | Planet appearance from orbit, then local repeated ground texture |
| Elevation map (`p_surfacemap`) | 200×200 bytes | Heights for the roughly 6.55 km square local sector |
| Object map (`objectschart`) | 200×200 packed bytes | Object count and three two-bit object classes per terrain cell |

The F02 system properties select planet-wide surface seed `952631`. The local
terrain PRNG is then reseeded from the landing-coordinate product; for
`001:060`, the 16-bit landing seed is `60`. Environment selection precedes
`build_surface()`, which builds the local height, texture, and object maps and
then applies FELYSIA's special historical-ruin branch.

## Canonical FELYSIA result

The headless `representative_surface_fixtures` CTest creates the tracked F02
save in an isolated runtime and invokes
`nivlr --surface-fixture felysia-habitable`. All hashes are FNV-1a
over raw logical bytes. The probe fixes the simulation epoch before orbital and
surface setup so later wall-clock dates cannot change the baseline:

| Field | Exact native result |
| --- | --- |
| Global surface seed | `952631` |
| Scenario | `2` (plains / vegetated) |
| Ground texture scale | `128` |
| 40,000-byte elevation hash | `da14eccf19e9fc07` |
| 65,536-byte ground-texture hash | `eac1bb14fcf7df3f` |
| 40,000-byte object-map hash | `5ea2525745b31bde` |
| Elevation byte range | `48..131` |

Clang, GCC, and Clang ASan/UBSan agree exactly. Repeated runs in fresh isolated
runtimes also agree.

## Representative landable classes

M4-W02 extends the same full-buffer probe across every planet type the ship is
allowed to land on. Six cases are generated bodies in the DOS-confirmed
FELYSIA system. Type 7 comes from the audited class-11 system-generator branch,
which reliably provides an icy body. The checked-in `native_surfaces.tsv`
contains every exact seed, scenario, scale, hash, and height range.

| Type | Fixture | Surface family |
| --- | --- | --- |
| 1 | `felysia-rocky` | Airless, cratered rock |
| 2 | `felysia-thick-atmosphere` | Thick-atmosphere / Venus-like |
| 3 | `felysia-habitable` | Habitable plains at the F02C coordinate |
| 4 | `felysia-corrugated` | Corrugated rocky terrain |
| 5 | `felysia-thin-atmosphere` | Thin-atmosphere / Mars-like |
| 7 | `class11-icy` | Striated icy terrain |
| 8 | `felysia-milky` | Milky surface |

Types 0, 6, 9, and 10 are deliberately absent: the live landing controls
reject volcanic worlds, gas giants, substellar objects, and companion stars.
They are generated and rendered from orbit but do not have mandatory explorable
surface paths.

## Portability repairs found during characterization

- The documented one-byte object record used 16-bit bitfield storage on modern
  compilers. It now uses eight-bit bitfields and has a size assertion.
- FELYSIA ruin averaging used a signed 16-bit map index, which wrapped above
  cell 32,767, and a signed 16-bit accumulator that could overflow. The index
  now covers all 40,000 cells and the average accumulates in 32 bits.
- The final slope/object-density scan read beyond the logical height-map border.
  It now stops before the required right/bottom neighbors leave the map.
- Orbital texture generation converted large rotation values and negative
  cyclone coordinates directly to narrow integers. Explicit modulo behavior
  now preserves the intended legacy wrapping without C++ undefined behavior.
- The rocky-world orbital texture used a 320-pixel border allowance while
  smoothing a 360-pixel-wide map, reading 40 bytes beyond its allocation. The
  loop now stops before its actual one-row-and-one-column lookahead.
- Crater and permanent-storm coordinates can legitimately cross the left edge
  before wrapping in the original 16-bit arithmetic. They now use the explicit
  legacy unsigned conversion rather than undefined C++ float-to-integer casts.

These repairs precede the first accepted full-buffer native hashes; no unstable
pre-fix output has been blessed as compatibility evidence.

## Comparison level and limits

F02C proves the landing coordinate, serialized surface state, global/local
seeds, a structurally vegetated surface, and a perceptually green-dominant DOS
view. The three buffer hashes above are **EXACT native regression fixtures**.
They are **not** DOS byte goldens: the DOS buffers have not been extracted.

The matrix now characterizes every landable planet family, but only the
FELYSIA type-3 observations are DOS-backed. M4-W03 and later work own landing
transitions, complete live surface rendering, weather, and interactive
exploration.
