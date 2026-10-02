#include "legacy_prng.h"

#include "brtl.h"

std::uint32_t flat_rnd_seed;

void fast_srand(std::uint32_t seed) { flat_rnd_seed = seed | UINT32_C(0x03); }

std::int32_t fast_random(std::int32_t mask) {
    const std::uint64_t product = static_cast<std::uint64_t>(flat_rnd_seed) * flat_rnd_seed;
    std::uint32_t low = static_cast<std::uint32_t>(product);
    const auto high = static_cast<std::uint32_t>(product >> 32u);
    const auto combined_low_byte = static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(low) + static_cast<std::uint8_t>(high));
    low = (low & UINT32_C(0xffffff00)) | combined_low_byte;
    flat_rnd_seed += low;
    return static_cast<std::int32_t>(low & static_cast<std::uint32_t>(mask));
}

std::int16_t ranged_fast_random(std::int16_t range) {
    if (range <= 0) {
        range = 1;
    }
    return static_cast<std::int16_t>(fast_random(0x7fff) % range);
}

float flandom() { return static_cast<float>(brtl_random(32767)) * 0.000030518F; }

float fast_flandom() { return static_cast<float>(fast_random(32767)) * 0.000030518F; }
