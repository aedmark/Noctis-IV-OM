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
platform-specific buffer hashes. Since the M8-W01 HUD formatting fix (see
"Baseline refresh after M8-W01" below), three of five live environment frames
and the rocky populated sequence are byte-identical between the Release builds;
the remaining two environment frames, the habitable populated sequence, and
the three surface checkpoints in the journey have platform-specific
indexed-frame hashes.

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

## Baseline refresh after M8-W01

The Windows package workflow failed from commit `713a66d` (M8-W01) onward
because the MSVC Release surface baselines no longer matched. That commit
replaced `gcvt` with `snprintf("%.15g")` in `alphavalue`. The surface HUD
prints its `SQC` longitude, latitude, and position readout through
`alphavalue`, and MSVC's `_gcvt` writes a trailing decimal point for integral
values (`1.` where glibc writes `1`). The M7 MSVC baselines therefore recorded
HUD text that differed from Linux; glibc formats both ways identically, so the
Linux baselines were unaffected.

Probe run `37065718049` built `windows-msvc-release` from master `2a67147` in
two variants and printed every fixture's output. With `alphavalue` reverted to
`_gcvt`, all eight previous MSVC Release hashes (five environment frames, two
populated sequences, and the journey's ground, outbound, and capsule
checkpoints) were reproduced exactly. This shows that no other surface change
since M7 alters the MSVC Release output: not the 1.0.1 dim-star illumination,
the 1.1.0 torch, or the 1.5.0 sub-pixel work. With the current code, every
structural field and count is unchanged. The `habitable 4:60`, `habitable 1:0`,
and `rocky 1:60` environment frames and the rocky populated sequence now equal
the Linux baselines exactly. The MSVC manifests were refreshed to the
current-code hashes from that run. The remaining differences are the inherited
floating-point differences described above.
