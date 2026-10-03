# Known issues and accepted preview differences

This preview has one accepted cross-platform presentation difference:

- Linux Clang/GCC and Windows MSVC do not produce byte-identical surface
  buffers or final indexed frames in every scene. The inherited generator and
  software renderer use long floating-point and transcendental calculations;
  small compiler-specific rounding differences can change terrain detail,
  texture indices, and rasterized edge pixels.
- The verified platform baselines retain the same selected worlds, scenarios,
  weather states, flight and landing timing, navigation state, tree/rock/animal/
  ruin/capsule draw counts, and save behavior. Four of seven raw generator cases
  are byte-identical between Linux and Windows Release builds. Three have
  platform-specific exact hashes. All accepted Release hashes are enforced in
  CI rather than ignored.
- This is a visual/terrain-detail compatibility difference, not a save-format
  split. Saves and catalogs remain portable between the supported builds.

Other preview limitations:

- The Windows ZIP is unsigned, so Windows may show a SmartScreen warning.

Please include the build checksum and diagnostics output with any report. See
`COMMUNITY_TESTING.md` for the focused test route and report link.
