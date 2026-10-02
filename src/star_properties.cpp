#include "star_properties.h"

#include "brtl.h"
#include "legacy_numeric.h"
#include "noctis-d.h"

int8_t class_rgb[3 * star_classes] = {63, 58, 40, 30, 50, 63, 63, 63, 63, 63, 30, 20, 63, 55, 32, 32, 16, 10,
                                      32, 28, 24, 10, 20, 63, 63, 32, 16, 48, 32, 63, 40, 10, 10, 00, 63, 63};

int16_t class_ray[star_classes] = {5000, 15000, 300, 20000, 15000, 1000, 3000, 2000, 4000, 1500, 30000, 250};

int16_t class_rayvar[star_classes] = {2000, 10000, 200, 15000, 5000, 1000, 3000, 500, 5000, 10000, 1000, 10};

StarProperties derive_star_properties(double x, double y, double z) {
    const double identity = x / 100000 * y / 100000 * z / 100000;
    // Float-to-uint16 conversion of a negative identity is undefined in C++.
    // Truncate first, then use the intended 16-bit modulo seed.
    brtl_srand(legacy_u16_from_double(identity));

    StarProperties result{};
    result.star_class   = brtl_random(star_classes);
    result.radius_milli = class_ray[result.star_class] + brtl_random(class_rayvar[result.star_class]);
    // Preserve the original float-to-double multiplication and float assignment.
    result.radius = static_cast<float>(static_cast<float>(result.radius_milli) * 0.001);
    result.red    = class_rgb[3 * result.star_class + 0];
    result.green  = class_rgb[3 * result.star_class + 1];
    result.blue   = class_rgb[3 * result.star_class + 2];

    if (result.star_class == 11) {
        result.spin = brtl_random(30) + 1;
    }
    if (result.star_class == 7) {
        result.spin = brtl_random(12) + 1;
    }
    if (result.star_class == 2) {
        result.spin = brtl_random(4) + 1;
    }

    return result;
}
