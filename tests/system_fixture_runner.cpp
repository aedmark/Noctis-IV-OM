#include "system_properties.h"

#include <bit>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime        = 1099511628211ULL;

template <typename Unsigned>
void hash_unsigned(std::uint64_t &hash, Unsigned value) {
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        hash ^= static_cast<std::uint8_t>(value >> (byte * 8));
        hash *= fnv_prime;
    }
}

std::uint64_t system_fingerprint(const PlanetSystemProperties &system) {
    std::uint64_t hash = fnv_offset_basis;
    hash_unsigned(hash, static_cast<std::uint16_t>(system.planet_count));
    hash_unsigned(hash, static_cast<std::uint16_t>(system.body_count));
    for (std::int16_t index = 0; index < system.body_count; ++index) {
        const auto &body = system.bodies[index];
        hash_unsigned(hash, static_cast<std::uint8_t>(body.type));
        hash_unsigned(hash, static_cast<std::uint16_t>(body.owner));
        hash_unsigned(hash, static_cast<std::uint8_t>(body.moon_id));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.ring_radius));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.tilt));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.radius));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.orbit_radius));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.orbit_seed));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.orbit_tilt));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.orbit_orientation));
        hash_unsigned(hash, std::bit_cast<std::uint64_t>(body.orbit_eccentricity));
    }
    return hash;
}
} // namespace

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--emit")) {
        std::cerr << "usage: system_fixture_runner FILE [--emit]\n";
        return 2;
    }

    const bool emit = argc == 3;
    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "cannot open fixture file: " << argv[1] << '\n';
        return 2;
    }

    int cases = 0;
    int failures = 0;
    std::string line;
    for (int line_number = 1; std::getline(input, line); ++line_number) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream row(line);
        std::string id;
        double x, y, z;
        int star_class;
        float star_radius;
        int body_index;
        if (!(row >> id >> x >> y >> z >> star_class >> star_radius >> body_index)
            || body_index < 0 || body_index >= static_cast<int>(maximum_system_bodies)) {
            std::cerr << "malformed system fixture at line " << line_number << '\n';
            return 2;
        }

        const auto system = derive_planet_system(x, y, z, static_cast<std::int16_t>(star_class), star_radius);
        ++cases;
        if (body_index >= system.body_count) {
            std::cerr << id << ": requested body is absent\n";
            ++failures;
            continue;
        }
        const auto &body = system.bodies[body_index];
        const float revolution_period =
            derive_revolution_period(system, static_cast<std::int16_t>(body_index), star_radius);
        if (emit) {
            constexpr std::int16_t default_landing_longitude = 0;
            constexpr std::int16_t default_landing_latitude  = 60;
            std::cout << std::setprecision(17) << id << '\t' << x << '\t' << y << '\t' << z << '\t'
                      << star_class << '\t' << star_radius << '\t' << body_index << '\t' << system.planet_count
                      << '\t' << system.body_count << '\t' << static_cast<int>(body.type) << '\t' << body.owner
                      << '\t' << static_cast<int>(body.moon_id) << '\t' << body.radius << '\t' << body.orbit_radius
                      << '\t' << body.orbit_seed << '\t' << body.orbit_tilt << '\t' << body.orbit_orientation
                      << '\t' << body.orbit_eccentricity << '\t' << body.tilt << '\t' << body.ring_radius << '\t'
                      << revolution_period << '\t' << default_landing_longitude << '\t'
                      << default_landing_latitude << '\t'
                      << derive_global_surface_seed(body, default_landing_longitude, default_landing_latitude) << '\t'
                      << derive_landing_seed(default_landing_longitude, default_landing_latitude) << '\t'
                      << system_fingerprint(system) << '\n';
            continue;
        }

        int planet_count, body_count, type, owner, moon_id;
        double radius, orbit_radius, orbit_seed, orbit_tilt, orbit_orientation, orbit_eccentricity, tilt, ring_radius;
        float expected_revolution_period;
        int landing_longitude, landing_latitude;
        std::int32_t expected_global_surface_seed;
        unsigned int expected_landing_seed;
        std::uint64_t expected_fingerprint;
        std::string extra;
        if (!(row >> planet_count >> body_count >> type >> owner >> moon_id >> radius >> orbit_radius >> orbit_seed
                  >> orbit_tilt >> orbit_orientation >> orbit_eccentricity >> tilt >> ring_radius
                  >> expected_revolution_period >> landing_longitude >> landing_latitude
                  >> expected_global_surface_seed >> expected_landing_seed >> expected_fingerprint)
            || (row >> extra)) {
            std::cerr << "malformed system expectations at line " << line_number << '\n';
            return 2;
        }
        if (system.planet_count != planet_count || system.body_count != body_count || body.type != type
            || body.owner != owner || body.moon_id != moon_id || body.radius != radius
            || body.orbit_radius != orbit_radius || body.orbit_seed != orbit_seed || body.orbit_tilt != orbit_tilt
            || body.orbit_orientation != orbit_orientation || body.orbit_eccentricity != orbit_eccentricity
            || body.tilt != tilt || body.ring_radius != ring_radius
            || revolution_period != expected_revolution_period
            || derive_global_surface_seed(body, static_cast<std::int16_t>(landing_longitude),
                                          static_cast<std::int16_t>(landing_latitude))
                != expected_global_surface_seed
            || derive_landing_seed(static_cast<std::int16_t>(landing_longitude),
                                   static_cast<std::int16_t>(landing_latitude))
                != expected_landing_seed
            || system_fingerprint(system) != expected_fingerprint) {
            std::cerr << id << " (line " << line_number << "): system properties differ from fixture\n";
            ++failures;
        }
    }

    if (input.bad() || cases == 0) {
        std::cerr << "fixture file could not be read or has no cases\n";
        return 2;
    }
    if (!emit) {
        std::cout << "checked " << cases << " system bodies; " << failures << " mismatches\n";
    }
    return failures == 0 ? 0 : 1;
}
