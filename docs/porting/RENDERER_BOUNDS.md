# M17-W06 renderer and surface-map bounds

Status: COMPLETE

## Removed compatibility padding

Two allocations retained excess storage solely to absorb legacy accesses:

| Buffer | Logical capacity | Previous allocation | W06 allocation |
| --- | ---: | ---: | ---: |
| `adapted` maximum indexed framebuffer | 1,024,000 bytes (1280×800) | 1,089,536 bytes | 1,024,000 bytes |
| `p_surfacemap` height/scratch map | 40,000 bytes (200×200) | 105,536 bytes (`ps_bytes | 65536`) | 40,000 bytes |

Neither buffer now relies on adjacent address space. The active framebuffer is
usually 320×200, 640×400, or 1280×800; access is checked against the current
width and height, not merely the maximum allocation.

## Bounded renderer contract

The legacy texture mapper retains its 16-bit address arithmetic, mask, bias,
filtering, and exact in-range output. Its memory boundary is now explicit:

- `set_texture_source(pointer, capacity)` changes the source as one operation;
- every nearest, bilinear, and detailed sample validates the masked/bias-adjusted
  offset against that capacity; and
- an out-of-range texture sample returns indexed color zero.

All production source changes now declare the actual available bytes:
65,552 for the planetary/ground texture, 64,800 when that pointer aliases the
sky map, 32,768 for the globe/font bank, 40,000 for the full surface scratch
map, and 37,936 for its cockpit-screen subview. Recursive tree rendering and
font rendering save and restore both pointer and capacity.

The textured scanline writer now routes reads and writes through the active
framebuffer boundary. Out-of-range reads return zero and writes are discarded.
The 2D flare line path also rejects negative coordinates and handles a
single-point segment without division by zero.

## Surface generation repair

`felisian_srf_darkline()` previously validated a crevasse center and then
unconditionally wrote its left, right, upper, and lower neighbors. A center on
the final rows could write beyond cell 39,999; the oversized `p_surfacemap`
allocation hid that defect. Each neighbor is now checked independently. Every
legal write and all accepted surface hashes remain unchanged.

## Acceptance evidence

`renderer_fixtures` now uses an exact 64,000-byte active page followed by a
64-byte sentinel. It verifies rejection of the first out-of-range byte,
negative/off-page line clipping, an in-range one-byte texture, and an
out-of-range 16-bit texture address. Existing flat and textured framebuffer
hashes remain exact.

The complete Clang Debug suite passes 57/57. The complete Clang
AddressSanitizer/UndefinedBehaviorSanitizer suite also passes 57/57 with leak
detection disabled because LeakSanitizer cannot operate under the desktop
runner's ptrace policy. Representative surface generation, live environment,
content, landing, scripted journey, and recorded orbit-to-surface fixtures keep
their existing exact hashes and counters. GCC Debug also passes 57/57, and the
existing MinGW Windows and Emscripten Web Release trees compile and link.
