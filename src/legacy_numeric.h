#pragma once

#include <cstdint>

// DOS-era conversions relied on x86/Borland width and wrap behavior. Keep the
// modulo conversion explicit so modern floating-to-integer casts stay defined.
std::uint32_t legacy_u32_from_double(double value);
std::uint16_t legacy_u16_from_double(double value);
std::int32_t legacy_i32_from_double(double value);

// Resource integers are serialized little-endian and may be unaligned.
std::uint32_t read_u32_le(const std::uint8_t *bytes);
