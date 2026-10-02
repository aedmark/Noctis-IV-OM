# M3 space-rendering fixtures

Status: M3-W04 complete for the production software polygon paths on Linux.

## Fixture boundary

The `renderer_fixtures` CTest compiles the production `tdpolygs.h` renderer
into a headless fixture runner. It supplies the same 320×200 indexed buffer,
camera constants, projection setup, and texture layout used by the game. This
tests the logical framebuffer before palette conversion, Raylib upload, window
scaling, or GPU behavior.

Both fixtures use an identity camera at the origin, a 200-unit projection
plane, no flare blending, and a clockwise quad at `z = 500`:

| Fixture | Production path | Input | Exact result |
| --- | --- | --- | --- |
| Flat panel | `poly3d` | Color index 42; x ±120, y ±75 | 5,917 nonzero pixels; FNV-1a `f59885b32c4eda4f` |
| Textured panel | `polymap` | 256×256 procedural 16-pixel checker-gradient; same quad | 5,856 nonzero pixels; FNV-1a `f64a31f3c71b46e3` |

The hashes cover all 64,000 visible index bytes, including untouched
background, polygon boundaries, fill, projection, and texture sampling. Pixel
counts make an all-background or accidental hash update conspicuous. The
procedural texture avoids binary fixture provenance and host image-decoder
differences.

Clang, GCC, and Clang ASan/UBSan must produce the same exact hashes. The
sanitized runner directly exercises the texture-mapping code that the earlier
startup smoke did not reach.

## Comparison level and limits

These are `EXACT` native renderer regression fixtures. They establish compiler
agreement and protect the inherited flat/textured algorithms during the M3
port; they are not represented as DOS pixel captures. DOS F04 snapshot/control
capture remains separate compatibility evidence and can later classify full
scenes perceptually or exactly at the indexed-buffer boundary.

The fixtures do not yet cover clipping across the near plane, flare blend
modes, panoramic snapshots, anti-alias settings, whole Stardrifter geometry, or
host scaling. M3-W05/W06 will exercise complete travel scenes, while F04 and
the NIV+ rendering ledger retain the remaining presentation modes.
