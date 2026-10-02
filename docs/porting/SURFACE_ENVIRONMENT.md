# M4-W04 Surface Environment Rendering

M4-W04 protects the visible environment around a landed explorer. The
headless `surface_environment_fixtures` test generates a real surface, enters
the production exploration loop, renders one complete 320×200 indexed frame,
and fingerprints all 64,000 pixels.

The five native cases divide the important environment branches:

| World and coordinates | Path exercised |
| --- | --- |
| FELYSIA `LQ 001:060` | rainy plains, terrain, sky, atmosphere, and rain |
| FELYSIA `LQ 004:060` | open ocean plus incoming and outgoing wave renderers |
| FELYSIA `LQ 001:000` | polar ice with dry weather |
| thick-atmosphere P08 `LQ 001:060` | dense atmospheric overlay and wind |
| rocky P02 `LQ 001:060` | airless terrain and black-sky path |

Every case resets the fixed simulation clock to zero before generation. This
makes weather animation, seeded rendering, and the resulting frame independent
of wall-clock time and of earlier fixture processes. Clang, GCC, and the
Clang ASan/UBSan build produce the exact hashes in
`modern/tests/fixtures/native_environments.tsv`.

The live ocean frame exposed a legacy texture address that could select the
upper half of a 16-bit offset even though the wave gradient occupies only the
first 32 KiB of its aliased map. The renderer now applies the intended 15-bit
address mask while either wave path uses that map. The broader live frame also
made off-screen perspective conversion and interpolation wrapping explicit,
matching the old x86 integer behavior without undefined C++ conversions.

This is exact native regression evidence, not DOS pixel evidence. F02C confirms
FELYSIA and its selected landing sector structurally and perceptually, but no
DOS framebuffer was extracted for these five scenes.

Flora, fauna, ruins, loose objects, and the capsule model are intentionally
suppressed in this fixture. Their setup and rendering remain M4-W05 so failures
can be attributed to the environmental or living/object layer independently.
