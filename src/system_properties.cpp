#include "system_properties.h"

#include "brtl.h"
#include "legacy_numeric.h"

#include <cmath>

namespace {
constexpr std::array<std::int16_t, 12> class_planets = {12, 18, 8, 15, 20, 3, 0, 1, 7, 20, 2, 5};
constexpr std::array<std::int16_t, 11> possible_moons = {1, 1, 2, 3, 2, 2, 18, 2, 3, 20, 20};
constexpr std::array<double, 11> average_radius = {0.007, 0.003, 0.010, 0.011, 0.010, 0.008,
                                                   0.064, 0.009, 0.012, 0.125, 5.000};
constexpr double degrees             = 3.14159265358979323846 / 180.0;
constexpr double planet_orbit_scale  = 5.0;
constexpr double average_planet_size = 2.4;
constexpr double moon_orbit_scale    = 12.8;
constexpr double average_moon_size   = 1.8;

float zero_centered_random(std::int16_t range) {
    const std::int16_t left  = brtl_random(range);
    const std::int16_t right = brtl_random(range);
    return static_cast<float>(left - right);
}

std::uint16_t coordinate_seed(double x, double y, double z) {
    std::int64_t seed = legacy_i32_from_double(x) % 10000;
    seed = (seed * legacy_i32_from_double(y)) % 10000;
    seed = (seed * legacy_i32_from_double(z)) % 10000;
    return static_cast<std::uint16_t>(seed);
}
} // namespace

PlanetSystemProperties derive_planet_system(double x, double y, double z, std::int16_t star_class,
                                            float star_radius) {
    PlanetSystemProperties system;
    if (star_class < 0 || star_class >= static_cast<std::int16_t>(class_planets.size())) {
        return system;
    }

    brtl_srand(coordinate_seed(x, y, z));
    system.planet_count = brtl_random(class_planets[star_class] + 1);

    for (std::int16_t n = 0; n < system.planet_count; ++n) {
        auto &body             = system.bodies[n];
        body.owner             = -1;
        body.orbit_orientation = degrees * brtl_random(360);
        body.orbit_seed = 3.0 * (n * n + 1) * star_radius
            + static_cast<float>(brtl_random(static_cast<std::int16_t>(300 * star_radius))) / 100.0F;
        body.tilt = zero_centered_random(static_cast<std::int16_t>(10 * body.orbit_seed)) / 500.0;
        body.orbit_tilt = zero_centered_random(static_cast<std::int16_t>(10 * body.orbit_seed)) / 5000.0;
        body.orbit_eccentricity =
            1 - static_cast<double>(brtl_random(static_cast<std::int16_t>(body.orbit_seed
                                                   + 10 * std::fabs(body.orbit_tilt))))
                / 2000;
        body.radius = static_cast<double>(brtl_random(static_cast<std::int16_t>(body.orbit_seed))) * 0.001 + 0.01;
        body.ring_radius = zero_centered_random(static_cast<std::int16_t>(body.radius))
            * (1 + static_cast<double>(brtl_random(1000)) / 100.0);

        if (star_class != 8) {
            body.type = brtl_random(10);
        } else if (brtl_random(2)) {
            body.type = 10;
            body.orbit_tilt *= 100;
        } else {
            body.type = brtl_random(10);
        }

        if (star_class == 2 || star_class == 7 || star_class == 15) {
            body.orbit_seed *= 10;
        }
    }

    if (star_class == 0) {
        if (brtl_random(4) == 2) {
            system.bodies[2].type = 3;
        }
        if (brtl_random(4) == 2) {
            system.bodies[3].type = 3;
        }
        if (brtl_random(4) == 2) {
            system.bodies[4].type = 3;
        }
    }

    for (std::int16_t n = 0; n < system.planet_count; ++n) {
        auto &type = system.bodies[n].type;
        switch (star_class) {
        case 2:
            while (type == 3) {
                type = brtl_random(10);
            }
            break;
        case 5:
            while (type == 6 || type == 9) {
                type = brtl_random(10);
            }
            break;
        case 7:
            type = 9;
            break;
        case 9:
            while (type != 0 && type != 6 && type != 9) {
                type = brtl_random(10);
            }
            break;
        case 11:
            while (type != 1 && type != 7) {
                type = brtl_random(10);
            }
            break;
        default:
            break;
        }
    }

    for (std::int16_t n = 0; n < system.planet_count; ++n) {
        auto &type = system.bodies[n].type;
        switch (type) {
        case 0:
            if (brtl_random(8)) {
                ++type;
            }
            break;
        case 3:
            if (n < 2 || n > 6 || (star_class != 0 && brtl_random(4))) {
                type += brtl_random(2) ? 1 : -1;
            }
            break;
        case 7:
            if (n < 7) {
                type -= brtl_random(2) ? 1 : 2;
            }
            break;
        default:
            break;
        }
    }

    system.body_count = system.planet_count;
    if (star_class != 2 && star_class != 7 && star_class != 15) {
        for (std::int16_t n = 0; n < system.planet_count; ++n) {
            const std::int16_t parent_type = system.bodies[n].type;
            std::int16_t moon_count = 0;
            if (n < 2) {
                if (parent_type == 10) {
                    moon_count = brtl_random(3);
                }
            } else {
                moon_count = brtl_random(possible_moons[parent_type] + 1);
            }
            if (system.body_count + moon_count > static_cast<std::int16_t>(maximum_system_bodies)) {
                moon_count = static_cast<std::int16_t>(maximum_system_bodies) - system.body_count;
            }

            for (std::int16_t c = 0; c < moon_count; ++c) {
                const std::int16_t q = system.body_count + c;
                auto &moon           = system.bodies[q];
                moon.owner           = n;
                moon.moon_id         = static_cast<std::int8_t>(c);
                moon.orbit_orientation = degrees * brtl_random(360);
                moon.orbit_seed = (c * c + 4) * system.bodies[n].radius
                    + zero_centered_random(static_cast<std::int16_t>(300 * system.bodies[n].radius)) / 100;
                moon.tilt = zero_centered_random(static_cast<std::int16_t>(10 * moon.orbit_seed)) / 50;
                moon.orbit_tilt = zero_centered_random(static_cast<std::int16_t>(10 * moon.orbit_seed)) / 500;
                moon.orbit_eccentricity =
                    1 - static_cast<double>(brtl_random(static_cast<std::int16_t>(moon.orbit_seed
                                                       + 10 * std::fabs(moon.orbit_tilt))))
                        / 2000;
                moon.radius = static_cast<double>(brtl_random(static_cast<std::int16_t>(system.bodies[n].orbit_seed)))
                        * 0.05
                    + 0.1;
                moon.type = brtl_random(10);
                std::int8_t type = moon.type;

                if (type == 9 && parent_type != 10) {
                    type = 2;
                }
                if (type == 6 && parent_type < 9) {
                    type = 5;
                }
                if (n > 7 && brtl_random(c)) {
                    type = 7;
                }
                if (n > 9 && brtl_random(c)) {
                    type = 7;
                }
                if ((type == 2 || type == 3 || type == 4 || type == 8) && parent_type != 6 && parent_type < 9) {
                    type = 1;
                }
                if (type == 3 && parent_type < 9) {
                    if (n > 7) {
                        type = 7;
                    }
                    if (star_class != 0 && brtl_random(4)) {
                        type = 5;
                    }
                    if (star_class == 2 || star_class == 7 || star_class == 11) {
                        type = 8;
                    }
                }
                if (type == 7 && n <= 5) {
                    type = 1;
                }
                if ((star_class == 2 || star_class == 5 || star_class == 7 || star_class == 11)
                    && brtl_random(n)) {
                    type = 7;
                }
                moon.type = type;
            }
            system.body_count += moon_count;
        }
    }

    double key_radius = star_radius * planet_orbit_scale;
    if (star_class == 8) {
        key_radius *= 2;
    }
    if (star_class == 2) {
        key_radius *= 16;
    }
    if (star_class == 7) {
        key_radius *= 18;
    }
    if (star_class == 11) {
        key_radius *= 20;
    }

    for (std::int16_t n = 0; n < system.planet_count; ++n) {
        auto &body = system.bodies[n];
        body.radius = average_radius[body.type] + average_radius[body.type] * zero_centered_random(100) / 200;
        body.radius *= average_planet_size;
        body.orbit_radius = key_radius + key_radius * zero_centered_random(100) / 500;
        body.orbit_radius += key_radius * average_radius[body.type];
        key_radius += (n < 8 ? 1.0 : 0.22) * body.orbit_radius;
    }

    std::int16_t n = system.planet_count;
    while (n < system.body_count) {
        std::int16_t moon_index = 0;
        const std::int16_t owner = system.bodies[n].owner;
        key_radius               = system.bodies[owner].radius * moon_orbit_scale;
        while (n < system.body_count && system.bodies[n].owner == owner) {
            auto &moon = system.bodies[n];
            moon.radius = average_radius[moon.type] + average_radius[moon.type] * zero_centered_random(100) / 200;
            moon.radius *= average_moon_size;
            moon.orbit_radius = key_radius + key_radius * zero_centered_random(100) / 250;
            moon.orbit_radius += key_radius * average_radius[moon.type];
            if (moon_index < 2) {
                key_radius += moon.orbit_radius;
            } else if (moon_index < 8) {
                key_radius += 0.12 * moon.orbit_radius;
            } else {
                key_radius += 0.025 * moon.orbit_radius;
            }
            ++moon_index;
            ++n;
        }
    }

    for (n = 0; n < system.planet_count; ++n) {
        auto &body = system.bodies[n];
        body.ring_radius = 0.75 * body.radius * (2 + brtl_random(3));
        if (body.type != 6 && body.type != 9) {
            if (brtl_random(5)) {
                body.ring_radius = 0;
            }
        } else if (brtl_random(2)) {
            body.ring_radius = 0;
        }
    }

    return system;
}

float derive_revolution_period(const PlanetSystemProperties &system, std::int16_t body_index, float star_radius) {
    if (body_index < 0 || body_index >= system.body_count) {
        return 0;
    }
    const auto &body = system.bodies[body_index];
    const double orbit_squared = body.orbit_radius * body.orbit_radius;
    constexpr double sphere_volume_factor = 4.0 * 3.14159265358979323846 / 3.0;
    double mass;
    if (body.owner > -1) {
        const double parent_radius = system.bodies[body.owner].radius;
        mass = sphere_volume_factor * parent_radius * parent_radius * parent_radius * 0.44e-4;
    } else {
        mass = sphere_volume_factor * star_radius * star_radius * star_radius * 0.01e-7;
    }
    return static_cast<float>(360 / std::sqrt(mass / orbit_squared));
}

std::int32_t derive_global_surface_seed(const PlanetBodyProperties &body, std::int16_t landing_longitude,
                                        std::int16_t landing_latitude) {
    std::int32_t seed = legacy_i32_from_double(
        (body.radius + body.orbit_radius + body.orbit_orientation) * 4112.0);
    if (body.type == 3) {
        brtl_srand(static_cast<std::uint16_t>(seed + landing_longitude));
        const float latitude = static_cast<float>(std::abs(landing_latitude - 60)) * 1.5F;
        if (latitude > 25 + (seed % 15) + brtl_random(5)) {
            ++seed;
        }
    }
    return seed;
}

std::uint16_t derive_landing_seed(std::int16_t landing_longitude, std::int16_t landing_latitude) {
    return static_cast<std::uint16_t>(static_cast<std::int32_t>(landing_longitude) * landing_latitude);
}
