#include "legacy_numeric.h"

#include <cmath>

std::uint32_t legacy_u32_from_double(double value) {
    if (!std::isfinite(value)) {
        return 0;
    }

    constexpr double modulus = 4294967296.0;
    double wrapped = std::fmod(std::trunc(value), modulus);
    if (wrapped < 0) {
        wrapped += modulus;
    }
    return static_cast<std::uint32_t>(wrapped);
}

std::uint16_t legacy_u16_from_double(double value) {
    return static_cast<std::uint16_t>(legacy_u32_from_double(value));
}

std::int32_t legacy_i32_from_double(double value) {
    const std::uint32_t wrapped = legacy_u32_from_double(value);
    if (wrapped <= INT32_MAX) {
        return static_cast<std::int32_t>(wrapped);
    }
    return static_cast<std::int32_t>(static_cast<std::int64_t>(wrapped) - INT64_C(4294967296));
}

std::uint32_t read_u32_le(const std::uint8_t *bytes) {
    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8u)
        | (static_cast<std::uint32_t>(bytes[2]) << 16u)
        | (static_cast<std::uint32_t>(bytes[3]) << 24u);
}
