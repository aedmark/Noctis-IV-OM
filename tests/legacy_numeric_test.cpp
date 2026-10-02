#include "legacy_numeric.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {
bool expect(std::uint32_t actual, std::uint32_t expected, const char *name) {
    if (actual == expected) {
        return true;
    }
    std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
    return false;
}

bool expect_signed(std::int32_t actual, std::int32_t expected, const char *name) {
    if (actual == expected) {
        return true;
    }
    std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
    return false;
}
}

int main() {
    bool ok = true;
    ok &= expect(legacy_u32_from_double(4294967301.75), 5u, "positive wrap");
    ok &= expect(legacy_u32_from_double(-5.68182), UINT32_C(0xfffffffb), "negative wrap");
    ok &= expect(legacy_u32_from_double(12.99), 12u, "truncate fraction");
    ok &= expect(legacy_u16_from_double(65541.75), 5u, "16-bit positive wrap");
    ok &= expect(legacy_u16_from_double(-5.68182), UINT16_C(0xfffb), "16-bit negative wrap");
    ok &= expect_signed(legacy_i32_from_double(4294967291.75), -5, "signed positive wrap");
    ok &= expect_signed(legacy_i32_from_double(-5.68182), -5, "signed negative value");

    const std::array<std::uint8_t, 6> unaligned = {0xff, 0x78, 0x56, 0x34, 0x12, 0xff};
    ok &= expect(read_u32_le(unaligned.data() + 1), UINT32_C(0x12345678), "unaligned little-endian read");
    return ok ? 0 : 1;
}
