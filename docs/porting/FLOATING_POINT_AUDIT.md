# M2 floating-point and iteration-order audit

Status: M2-W05 audit complete for the extracted galaxy, star, system, period,
and surface-seed compatibility paths. This is not an audit of rendering,
terrain generation, simulation timing, or persistence; those paths belong to
later milestones.

## Compatibility policy

Noctis generation mixes a 16-bit Borland-compatible PRNG with integer
truncation, `float` temporaries, `double` accumulation, and stateful iteration.
Reassociation or an apparently harmless loop reordering can therefore change
all subsequent bodies. The native port follows these rules:

1. Preserve the source expression grouping and loop order where PRNG state or
   accumulated radii are involved.
2. Make DOS-width truncation and modulo behavior explicit before it reaches
   modern C++ undefined or implementation-sensitive conversions.
3. Compile compatibility-sensitive C++ without fused multiply-add contraction.
4. Compare exact integer and IEEE bit patterns for native regression fixtures.
5. Treat only repeated DOS captures as external compatibility proof. A stable
   Clang/GCC fingerprint is a native regression baseline, not DOS evidence.

## Audited calculations

| Path | Sensitivity | Preserved behavior | Evidence |
| --- | --- | --- | --- |
| Galaxy sector selection | Signed overflow and high/low product folding | `galaxy_sector.cpp` uses explicit modulo-32-bit add/subtract and the legacy folded multiply; no floating point is involved | Galaxy fixture covers signs, zero, rarity outcomes, and all empty-axis filters |
| Star identity and PRNG seed | Division/multiplication association can cross an integer boundary before 16-bit seed truncation | `x / 100000 * y / 100000 * z / 100000` remains left-associated | `association-boundary` evaluates to `58129314820.0`; a regrouped product is below that integer and produces a different seed |
| Star radius | Original calculation adds integer milliradii, converts to `float`, multiplies by a `float` approximation of `0.001`, then stores `float` | `derive_star_properties()` retains the two explicit `float` conversions | Four repeated DOS star references compare exact radius bits |
| System coordinate seed | Coordinate truncation, signed remainder, multiplication, and remainder order affect the 16-bit seed | Each coordinate is converted with explicit legacy i32 semantics; multiplication and `% 10000` occur left-to-right in a widened integer | FELYSIA and full-system fingerprints |
| Initial body draw | Mixed `float` and `double` operands determine later integer PRNG ranges | The extracted runner mirrors the original casts and statement order | Full-system fingerprints cover every body field after the complete PRNG sequence |
| Planet orbit normalization | `key_radius` is a stateful sum; planets after index 7 use a `0.22` increment | Bodies remain sequential and the accumulated radius is updated only after the current orbit is finalized | `late-orbits` covers 19 planets and 70 total bodies |
| Moon generation and normalization | Moon count changes later PRNG calls; moons are grouped by owner; increment factors change after moon indices 1 and 7 | Owner-major generation and normalization order is retained, including the legacy first-moon range behavior | Class 8 and class 11 fingerprints cover 23 and 36 bodies; FELYSIA covers 22 |
| Class-specific filtering/scaling | Rejection loops consume a variable number of PRNG draws; classes 2, 7, 8, 9, and 11 have distinct branches | Rejection and scale branches remain in source order | Dedicated native-only fingerprints for each class |
| Revolution period | Operation grouping, the legacy `4*pi/3` volume factor, `sqrt`, and final `float` rounding affect displayed seconds | Period is evaluated in `double`, then converted once to `float` | F02B repeats DOS text `111:753:904`; native value is exactly `111753904.0f` |
| Surface seeds | Addition order and double-to-i32 truncation select the global terrain seed; longitude/latitude product wraps to 16 bits | Seed derivation uses explicit legacy conversion and preserves the type-3 latitude adjustment draw | F02C matches global seed `952631` and landing seed `60` |

## Full-system fingerprint

`system_fixture_runner` hashes each generated system in body-generation order.
The fingerprint includes planet/body counts and, for every body, type, owner,
moon index, ring radius, tilt, normalized radius, orbit radius, initial orbit
seed, orbit tilt, orientation, and eccentricity. Integers and IEEE-754 bit
patterns are fed to FNV-1a in an explicit little-endian order, so host object
padding and endianness cannot affect the result.

The matrix includes:

- FELYSIA's DOS-backed class-0 system;
- no-moon and large-orbit scaling for classes 2 and 7;
- class-8 companion generation;
- class-9 type rejection;
- class-11 scaling plus a 36-body moon sequence; and
- a 19-planet, 70-body system that crosses the late-orbit accumulation branch.

Clang and GCC must match every selected-body field and every full-system
fingerprint. The sanitized Clang lane exercises the same comparisons.

## Compiler controls

The native executable and the star, galaxy, system, and DOS-reference fixture
targets compile with `-ffp-contract=off` under Clang/GCC. The corresponding
MSVC setting is `/fp:strict`, but Windows remains unverified until M7-W01.
Neither the checked presets nor CI enable fast-math. These controls prevent an
available FMA instruction from silently changing a legacy multiply/add
boundary while retaining ordinary IEEE evaluation.

## Accepted limits and later work

- The DOS executable used x87 arithmetic in places. Four stars and FELYSIA's
  selected properties are grounded by repeated DOS observations; the broader
  bit-pattern matrix is native-only until separately captured.
- Transcendental rendering and orbital-position functions are outside the M2
  extracted property path. Their platform sensitivity belongs to M3/M4 visual
  and simulation fixtures.
- Surface terrain, flora/fauna, weather, and palette evolution are not covered
  by the two F02C seed assertions. They remain M4 work.
- Windows compiler and runtime behavior remains deferred by ADR-0009.
