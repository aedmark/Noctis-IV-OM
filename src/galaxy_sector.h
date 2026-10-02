#pragma once

#include <cstdint>
#include <optional>

struct GalaxyStar {
    int32_t x;
    int32_t y;
    int32_t z;
};

// Map one 100,000-unit sector to its procedural star, if it survives the
// legacy empty-axis and galactic-rarity filters. No renderer is required.
std::optional<GalaxyStar> galaxy_star_at(int32_t sector_x, int32_t sector_y, int32_t sector_z, int16_t rarity_mask);
