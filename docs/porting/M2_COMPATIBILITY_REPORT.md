# M2 deterministic compatibility report

Status: complete on the verified Linux toolchains. Windows is deferred to
M7-W01 by ADR-0009.

## Result

The extracted native galaxy, star, system, planet-property, and surface-seed
paths preserve the selected Noctis DOS universe identities. All exact fields
observed in repeated DOS captures match native output. The wider native matrix
is deterministic across Clang, GCC, and sanitized Clang.

| Evidence set | Coverage | Comparison | Result |
| --- | --- | --- | --- |
| F01A–F01D | ADELPHE, JEHOVABOH, MIRACLE, NEW FELYSIA | Coordinates, class, RGB, spin, and `float` radius bits | Exact match |
| F02A | FELYSIA parent relation | Parent coordinates and starmap identity relation | Exact match |
| F02B | FELYSIA P04 system properties | Planet count, type, radius, environment, revolution display | Exact match |
| F02C | FELYSIA `LQ 001:060` | Global and landing seeds; serialized landing state | Exact seeds/state; terrain appearance structural/perceptual |
| Native star matrix | All 12 classes, sign/zero axes, rarity and empty-axis branches | Integer values and IEEE radius bits | Exact across Linux lanes |
| Native system matrix | Seven systems, all generated fields, up to 70 bodies | Little-endian FNV-1a full-system fingerprints | Exact across Linux lanes |

## Generator and arithmetic controls

Both legacy generators are isolated in production modules and protected by
fixed-sequence tests. The tests cover Borland `rand` compatibility and the
original inline-assembly fast generator's 32-bit square, byte fold, state
update, mask, and seed behavior. The F01/F02 DOS fixtures then protect their
observable call order through complete selected star/system outcomes.

Compatibility-sensitive floating conversions use explicit truncate-and-wrap
helpers. The star identity expression keeps its original association, system
bodies remain in PRNG generation order, and compatibility targets disable FMA
contraction. See `NUMERIC_WIDTH_AUDIT.md` and `FLOATING_POINT_AUDIT.md`.

## Accepted limits

- DOS evidence is deliberately selected rather than exhaustive. Native-only
  matrix rows are regression protection, not claimed DOS observations.
- F02C protects landing state and seeds, not byte-exact terrain, rendering,
  flora/fauna, weather, or palette evolution; those remain M4 work.
- Save/resume parity (F03), rendering controls (F04), and complete GOESnet
  behavior (F05) are not M2 exit criteria.
- Windows/MSVC behavior remains unverified and explicitly deferred.

No known mismatch exists inside the stated M2 scope. Every known uncovered
area above is assigned to an existing fixture or roadmap milestone.
