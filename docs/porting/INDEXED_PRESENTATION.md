# M3 indexed-framebuffer presentation

Status: M3-W03 complete on Linux.

## Preserved pipeline

Noctis continues to render into a 320×200 buffer of 8-bit palette indices.
Presentation is split into three explicit stages:

1. Legacy drawing writes color indices to `adapted`.
2. Palette updates copy/filter 6-bit RGB entries into the active 256-color
   palette using the original VGA DAC range semantics.
3. A pure converter expands indices to RGBA bytes; Raylib only uploads and
   scales that result to the 1280×720 window.

The converter writes explicit R, G, B, A bytes rather than relying on the host
byte order of a packed `uint32_t`. A VGA channel value of 63 maps to 252, as in
the inherited implementation's multiply-by-four rule, and alpha is always 255.
The RGBA buffer is retained between frames rather than allocated per swap.

## Regression evidence

The headless `indexed_framebuffer` CTest verifies:

- a partial palette update and its original prefix rewrite behavior;
- independent red, green, and blue filters with integer truncation;
- clamping overbright palette source values to the six-bit maximum;
- palette-index lookup order;
- RGBA channel order and opaque alpha; and
- the 6-bit-to-8-bit intensity rule.

The ordinary sanitized graphical smoke verifies that the extracted converter
still reaches the active Raylib loop. M2 star/system fixtures remain unchanged.

## Scope limits

- This work preserves the indexed/palette transport. Canonical rendered scene
  comparisons are M3-W04.
- Raylib currently stretches 320×200 directly to 1280×720. Aspect-ratio,
  filtering, and viewport policy will be judged with those rendering fixtures;
  W03 does not silently choose a new policy.
- Snapshot modes and panoramic capture remain part of later rendering and NIV+
  feature verification.
- Windows texture upload remains deferred to M7-W01.
