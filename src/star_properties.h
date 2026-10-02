#pragma once

#include <cstdint>

struct StarProperties {
    int16_t star_class;
    int32_t radius_milli;
    float radius;
    int8_t red;
    int8_t green;
    int8_t blue;
    int8_t spin;
};

// Uses the legacy Borland RNG and leaves its state where the game would.
StarProperties derive_star_properties(double x, double y, double z);
