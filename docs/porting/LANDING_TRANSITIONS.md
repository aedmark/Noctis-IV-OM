# M4-W03 Landing and Return Transitions

M4-W03 protects the normal FELYSIA landing loop from the flight computer to a
completed capsule return. The headless `landing_return_fixture` uses the
production FCS command, surface generator, gravity and collision code, WASD
input adapter, capsule proximity test, sealing/lift sequence, and final ship
state restoration.

The scripted pilot selects `LQ 001:060`, touches down on frame 488, walks more
than 1,600 surface units away so recovery becomes eligible, and returns to the
capsule on frame 553. The sealed capsule completes its lift on frame 799.

The fixture requires all of the following:

- the FCS accepts the landing request only after local arrival;
- touchdown and walking use the generated production height map;
- returning to the capsule triggers the normal close-and-lift path rather than
  the Escape/save shortcut;
- the exact double-precision galactic ship position is restored;
- the cockpit player position resets to `(0, 0, -3100)`;
- no legacy `surface.bin` or native `surface.niv` resume file is left after a normal return.

Characterization found that `planetary_main()` saved the double-precision ship
coordinates in temporary `float` variables. Every surface visit therefore
shifted the ship slightly on return. The backups now retain `double` precision.

The fixture intentionally suppresses live surface drawing and animal setup.
It still runs generation, gravity, terrain collision, walking, and capsule
physics. Terrain/sky/water/weather rendering belongs to M4-W04; life and object
rendering belongs to M4-W05.

This is an exact native transition fixture. F02C confirms the selected world,
coordinate, and a replayable DOS surface state, but no DOS frame count for the
capsule journey has been captured.
