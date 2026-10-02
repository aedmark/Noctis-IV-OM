# M4-W06 Orbit-to-Surface Journey

M4-W06 composes the separately protected M4 systems into one continuous native
journey. The headless `orbit_surface_journey` starts with the seeded FELYSIA
system and uses the production flight-computer command to request landing at
`LQ 001:060`.

The scripted pilot then:

1. views FELYSIA from orbit and enters the landing selector;
2. rides the rendered capsule through 488 surface frames to touchdown;
3. walks more than 1,600 surface units away through the populated environment;
4. returns to the capsule on frame 538; and
5. completes the normal lift and ship restoration on frame 783.

Unlike the focused M4-W03 transition fixture, this journey does not suppress
surface rendering or animal setup. Across its 817 rendered presentations it
exercises terrain, rainy sky and atmosphere, trees, moving animals, historical
ruin ground, the capsule model and beacon, gravity, collision, walking, capsule
proximity, and lift. It pins full 320×200 indexed-frame hashes at orbit,
touchdown, maximum excursion, and return to the capsule. It also requires exact
ship-position restoration and no leftover legacy or native surface checkpoint after normal lift.

The integrated pass exposed three boundaries hidden by the smaller fixtures:

- the descending capsule needed the same explicit 15-bit texture address used
  by the landed capsule;
- animal animation needed explicit legacy-width conversion after multiplying
  the universe clock; and
- texture pointer shifts used by trees and reflections needed to be modeled as
  a wrapping 16-bit address bias rather than out-of-allocation C++ pointers.

Resetting the actual fixed simulation clock—not only its public `secs` fields—
makes animal movement and every visual checkpoint deterministic. Clang, GCC,
and Clang ASan/UBSan now agree exactly on the full result.

This is exact native workflow evidence. F02C supplies the DOS-confirmed parent,
planet, landing coordinate, seeds, vegetated appearance, and visible large
ruins, but no DOS framebuffer sequence or frame timing was captured. The test
therefore does not claim DOS-exact pixels or cadence.
