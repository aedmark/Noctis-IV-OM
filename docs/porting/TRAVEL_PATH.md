# M3-W05 Travel Path

M3-W05 extracts the Stardrifter's interstellar and local approach guidance from
the rendering loop into `modern/src/travel.cpp`. The live game still owns target
selection, moving planetary coordinates, status display, and power reserves;
both drive modes now call the same independently testable production guidance.

## Canonical journey

The fixture begins at the inherited new-session Stardrifter coordinates and
selects the DOS-backed F02 parent system:

- parent star: BALASTRACKONASTREYA at `(-18928, -29680, -67336)`;
- star class/radius: class 0, `5.021` dyams;
- local target: FELYSIA, generated body index 3 / displayed planet P04;
- target properties: type 3, radius `0.026135999999999996` dyams;
- orbital pose: the production `planet_xyz` result at simulation second zero.

The interstellar leg reaches the anti-radiation calibration radius in exactly
396 fixed simulation ticks. The local leg then reaches less than two planetary
radii from FELYSIA in exactly 392 ticks. The inherited power behavior consumes
three of the initial 120 lithium charges and ends at 19,788 kilodyams.

The test observes the charging, driving, parking, approach, and arrival phases.
It also checks direct-target calibration distance and remote range rejection.
Clang, GCC, and sanitized Clang agree on the tick counts and final state.

## Scope boundary

This is an exact native regression fixture over production travel math, backed
by the M2 DOS-confirmed system identity and FELYSIA properties. It does not
claim DOS-exact travel timing or a complete graphical journey. The local target
is held at its selected deterministic orbital pose; the live game continues to
pass the planet's updated position into the same guidance on every tick.

M3-W06 completes the scripted smoke journey through injectable controls, state
checkpoints, and selected indexed-frame visual checkpoints; see
`SCRIPTED_JOURNEY.md`.
