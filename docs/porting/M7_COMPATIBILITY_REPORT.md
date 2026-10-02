# M7 cross-platform compatibility report

Status: complete. Windows Release run `36869013306` passes the full suite and
packages the published baselines and known-issues document.

## Scope

The comparison covers the four formerly deferred presentation tests on Linux
Clang/GCC and Windows MSVC:

- seven raw surface-generator cases;
- five live environment frames;
- two populated three-frame surface sequences; and
- the complete orbit, landing, surface walk, capsule return, and restored-ship
  journey.

All fixtures execute the production generator, indexed framebuffer, surface
content paths, and scripted journey. They are native regression fixtures, not
claims of DOS pixel identity.

## Result

The supported Release builds agree on every semantic and structural field:
selected body and scenario, seed, texture scale, height range, weather and wave
state, frame and transition timing, restored ship position, and all tree, rock,
animal, ruin, and capsule draw counts. The orbit frame before landing is also
byte-identical (`dd14fcc6528cab25`).

They are not universally pixel-identical. Four of seven raw generator cases
match in full. The thick-atmosphere, habitable, and icy cases have one or more
platform-specific buffer hashes. All five live environment frames, both
populated frames, and the three surface checkpoints in the journey have
platform-specific indexed-frame hashes.

The exact Linux baselines remain in `modern/tests/fixtures/native_surfaces.tsv`,
`native_environments.tsv`, and `native_surface_content.tsv`. Exact MSVC Release
baselines are recorded in the adjacent `_msvc_release.tsv` files. The journey
fixture holds both named Release expectations. CI treats each supported Release
baseline as exact; the differences are published, not skipped or hidden behind
a tolerance.

## Interpretation

The remaining differences occur inside inherited floating-point surface
generation and software rasterization. `/fp:strict` preserves expression
boundaries under MSVC, while Linux disables contraction, but neither policy
makes separate compiler math libraries and optimizers produce identical
transcendental rounding. The evidence does not show a gameplay-state,
persistence, navigation, content-count, or timing divergence.

MSVC Debug also produces selected hashes different from both Release baselines.
It remains a developer diagnostic probe rather than a shipped artifact. The
portable Windows preview is the statically linked Release configuration, and
its complete presentation suite is required to pass against the published
MSVC Release baselines.

## Evidence and acceptance

Probe run `36867609915` records all four current MSVC Debug mismatches rather
than excluding them silently. The verified Windows Release artifact from run
`36866929028` was independently executed headlessly under Wine; it reproduced
the exact MSVC 19.44 Release hashes recorded in the fixture manifests while
preserving every structural invariant above. Fresh hosted run `36869013306` at
commit `6617d0a` then passed all 40 non-graphical Release tests, verified the
archive and its checksum, verified the extracted profile behavior and static
runtime boundary, and uploaded the candidate ZIP. Linux build/package runs
`36869013216` and `36869013197` pass from the same commit.

The accepted player-facing limitation ships as `KNOWN_ISSUES.md`. A future
change to the generator or rasterizer must either retain the appropriate exact
platform baseline or receive a new evidence-backed compatibility review; hashes
must not be refreshed merely to make CI green.
