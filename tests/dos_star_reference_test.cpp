#include "galaxy_sector.h"
#include "star_properties.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string_view>

// Every row was captured at least twice from the pinned DOS executable. See the
// corresponding docs/porting/fixtures/ manifest for provenance and offsets.
struct DosStarReference {
    std::string_view name;
    int32_t sector_x, sector_y, sector_z;
    int32_t x, y, z;
    double starmap_id;
    int16_t star_class;
    int32_t radius_milli;
    uint32_t radius_bits;
    int8_t red, green, blue, spin;
};

constexpr DosStarReference references[] = {
    {"adelphe", 3300000, -4400000, -1100000, 3352848, -4391963, -1106519, 16294.138881133307, 2, 454,
     UINT32_C(0x3ee872b0), 63, 63, 63, 1},
    {"jehovaboh", 3900000, -4300000, -1100000, 3897488, -4324932, -1025582, 17287.590242758615, 8, 5186,
     UINT32_C(0x40a5f3b6), 63, 32, 16, 0},
    {"miracle", 3900000, -5100000, -100000, 3979984, -5143407, -98451, 2015.3586769998592, 3, 31234,
     UINT32_C(0x41f9df3b), 63, 30, 20, 0},
    {"new-felysia", 6600000, -4800000, -2400000, 6555696, -4832326, -2337595, 74053.28031476615, 0, 6913,
     UINT32_C(0x40dd374c), 63, 58, 40, 0},
};

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fputs("usage: dos_star_reference_test NAME\n", stderr);
        return 2;
    }

    for (const auto &reference : references) {
        if (reference.name != argv[1]) {
            continue;
        }

        const auto star = galaxy_star_at(reference.sector_x, reference.sector_y, reference.sector_z, 0);
        if (!star || star->x != reference.x || star->y != reference.y || star->z != reference.z) {
            std::fprintf(stderr, "%s: DOS galaxy coordinates differ\n", argv[1]);
            return 1;
        }

        const double native_id = static_cast<double>(star->x) / 100000 * star->y / 100000 * star->z / 100000;
        if (std::abs(native_id - reference.starmap_id) >= 0.00001) {
            std::fprintf(stderr, "%s: starmap ID differs beyond DOS lookup tolerance\n", argv[1]);
            return 1;
        }

        const auto properties = derive_star_properties(star->x, star->y, star->z);
        if (properties.star_class != reference.star_class || properties.radius_milli != reference.radius_milli ||
            std::bit_cast<uint32_t>(properties.radius) != reference.radius_bits || properties.red != reference.red ||
            properties.green != reference.green || properties.blue != reference.blue ||
            properties.spin != reference.spin) {
            std::fprintf(stderr, "%s: DOS star properties differ\n", argv[1]);
            return 1;
        }
        return 0;
    }

    std::fprintf(stderr, "unknown DOS star reference: %s\n", argv[1]);
    return 2;
}
