#include "galaxy_sector.h"

#include <bit>

namespace {
constexpr uint32_t offset_mask  = 0x0001FFFFu;
constexpr int32_t center_offset = 50000;

int32_t wrap_add(int32_t left, int32_t right) {
    return std::bit_cast<int32_t>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

int32_t wrap_sub(int32_t left, int32_t right) {
    return std::bit_cast<int32_t>(static_cast<uint32_t>(left) - static_cast<uint32_t>(right));
}

int32_t folded_product(int32_t left, int32_t right) {
    // The original imul path adds the high and low 32-bit words modulo 2^32.
    const auto product = static_cast<uint64_t>(static_cast<int64_t>(left) * right);
    return std::bit_cast<int32_t>(static_cast<uint32_t>(product) + static_cast<uint32_t>(product >> 32u));
}
} // namespace

std::optional<GalaxyStar> galaxy_star_at(int32_t sector_x, int32_t sector_y, int32_t sector_z, int16_t rarity_mask) {
    const int32_t sector_sum = wrap_add(sector_x, sector_z);
    int32_t x = wrap_add(sector_x, static_cast<int32_t>(static_cast<uint32_t>(sector_sum) & offset_mask));
    if (x == center_offset) {
        return std::nullopt;
    }
    x = wrap_sub(x, center_offset);

    const int32_t first_fold   = folded_product(x, sector_sum);
    const int32_t second_input = wrap_add(sector_sum, first_fold);
    int32_t y = wrap_add(sector_y, static_cast<int32_t>(static_cast<uint32_t>(first_fold) & offset_mask));
    if (y == center_offset) {
        return std::nullopt;
    }
    y = wrap_sub(y, center_offset);

    const int32_t second_fold = folded_product(y, second_input);
    int32_t z = wrap_add(sector_z, static_cast<int32_t>(static_cast<uint32_t>(second_fold) & offset_mask));
    if (z == center_offset) {
        return std::nullopt;
    }
    z = wrap_sub(z, center_offset);

    const uint32_t net_position = static_cast<uint32_t>(x) + static_cast<uint32_t>(y) + static_cast<uint32_t>(z);
    if ((net_position & static_cast<uint32_t>(static_cast<int32_t>(rarity_mask))) != 0) {
        return std::nullopt;
    }
    return GalaxyStar{x, y, z};
}
