#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

constexpr std::size_t maximum_system_bodies = 80;

struct PlanetBodyProperties {
    std::int8_t type{};
    std::int16_t owner{-1};
    std::int8_t moon_id{};
    double ring_radius{};
    double tilt{};
    double radius{};
    double orbit_radius{};
    double orbit_seed{};
    double orbit_tilt{};
    double orbit_orientation{};
    double orbit_eccentricity{};
};

struct PlanetSystemProperties {
    std::int16_t planet_count{};
    std::int16_t body_count{};
    std::array<PlanetBodyProperties, maximum_system_bodies> bodies{};
};

// Replays the legacy Borland RNG call order used by prepare_nearstar().
PlanetSystemProperties derive_planet_system(double x, double y, double z, std::int16_t star_class,
                                            float star_radius);

// Return the legacy revolution period in seconds for a generated body.
float derive_revolution_period(const PlanetSystemProperties &system, std::int16_t body_index, float star_radius);

// Replays the deterministic seeds selected before terrain generation. The
// global seed describes the planet/latitude style; the landing seed describes
// the selected longitude/latitude sector.
std::int32_t derive_global_surface_seed(const PlanetBodyProperties &body, std::int16_t landing_longitude,
                                        std::int16_t landing_latitude);
std::uint16_t derive_landing_seed(std::int16_t landing_longitude, std::int16_t landing_latitude);
