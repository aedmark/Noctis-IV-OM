# M2 numeric-width audit

Status: M2-W02 complete for deterministic galaxy, star, system, and surface-seed
generation. Persistence behavior is inventoried here but remains a later
fixture target (F03), not an M2 compatibility claim.

## Closed boundaries

| Boundary | Required legacy behavior | Native implementation and evidence |
| --- | --- | --- |
| Borland-compatible PRNG state | Unsigned 32-bit multiply/add wrap; 15-bit result | `brtl.cpp`; fixed seed sequence and range tests |
| Fast surface PRNG state | Unsigned 32-bit square/add wrap, low-byte fold, caller mask, seed OR 3 | `legacy_prng.cpp`; two fixed sequences, seed alias, and range-clamp tests |
| Floating PRNG seeds | Truncate toward zero, then reduce modulo 2^16 or 2^32 | `legacy_numeric.cpp`; positive, negative, fractional, and wrap tests |
| Signed coordinate seeds | DOS-width signed conversion without modern out-of-range conversion behavior | `legacy_i32_from_double`; focused tests and system fixtures |
| Star identity seed | Preserve left-associated floating expression, then modulo 2^16 | `derive_star_properties`; association-boundary and DOS star fixtures |
| Star-face seed | Multiply identity by 12345, truncate, then modulo 2^16 | `load_starface`; shared conversion helper |
| Resource word reads | Little-endian 32-bit value without alignment assumptions | `read_u32_le`; deliberately unaligned test input |
| Fixture fingerprints | Host-independent field encoding | Explicit 8/16/32-bit values and IEEE bit patterns hashed little-endian |

The active M2 extraction uses `std::uint8_t`, `std::int16_t`, `std::uint16_t`,
`std::int32_t`, and `std::uint32_t` at compatibility boundaries. No M2 seed
or generator state depends on the host width of `int` or `long`.

## Serialized state inventory

The current save and surface-state code specifies legacy field sizes directly
in its `fread`/`fwrite` calls (1, 2, 4, or 8 bytes), and the corresponding
globals use fixed-width integer types or explicitly sized `float`/`double`.
F02C verifies the selected surface coordinates and seeds after a DOS replay.
This audit does not establish whole-file byte parity, host endianness support,
short-I/O handling, or save/resume parity. Those require the planned F03
capture and belong to later persistence work.

## Scope limits

- Rendering, terrain buffers, GOESnet database mutation, and gameplay timing
  are outside the extracted M2 generators.
- Files remain legacy little-endian formats; big-endian hosts are not claimed.
- Windows/MSVC verification is deferred to M7-W01 by ADR-0009.
