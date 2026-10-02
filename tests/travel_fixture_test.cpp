#include "system_properties.h"
#include "travel.h"

#include <array>
#include <cmath>
#include <cstdio>

namespace {

noctis::TravelPosition felysia_position(const PlanetBodyProperties &body, const noctis::TravelPosition &parent_star) {
    // At simulation second zero the production planet_xyz calculation starts
    // at this deterministic point on the generated orbit.
    const double orbital_z = body.orbit_radius * body.orbit_eccentricity;
    return {parent_star.x + orbital_z * std::sin(body.orbit_orientation), parent_star.y,
            parent_star.z + orbital_z * std::cos(body.orbit_orientation)};
}

} // namespace

int main() {
    bool ok = true;
    constexpr noctis::TravelPosition launch{3797120.0, -4352112.0, -925018.0};
    constexpr noctis::TravelPosition balastrackonastreya{-18928.0, -29680.0, -67336.0};
    constexpr double star_radius = 5.021;

    ok &= noctis::remote_target_in_range(balastrackonastreya);
    ok &= !noctis::remote_target_in_range({0.0, 100000000.0, 0.0});
    ok &= noctis::remote_arrival_radius(false, true, star_radius) == 44.0 * star_radius;
    ok &= noctis::remote_arrival_radius(true, true, star_radius) == 25000.0;

    auto ship                            = launch;
    auto remote_guidance                 = noctis::begin_travel(ship, balastrackonastreya);
    const double remote_initial_distance = remote_guidance.initial_distance;
    std::array<bool, 10> phases{};
    int power          = 20000;
    int charge         = 120;
    auto consume_power = [&](std::int16_t cost) {
        power -= cost;
        if (power <= 15000 && charge > 0) {
            --charge;
            power = 20000;
        }
    };
    int remote_ticks = 0;
    for (; remote_ticks < 200000; ++remote_ticks) {
        const auto step = noctis::advance_remote_travel(
            ship, balastrackonastreya, noctis::remote_arrival_radius(false, true, star_radius), remote_guidance);
        phases[static_cast<std::size_t>(step.phase)] = true;
        consume_power(step.power_cost);
        if (step.arrived) {
            break;
        }
    }
    const double remote_arrival_distance = noctis::travel_distance(ship, balastrackonastreya);
    ok &= remote_ticks == 396;
    ok &= remote_arrival_distance < 44.0 * star_radius;
    ok &= remote_initial_distance > 5.0E6;
    ok &= phases[static_cast<std::size_t>(noctis::TravelPhase::charging)];
    ok &= phases[static_cast<std::size_t>(noctis::TravelPhase::driving)];
    ok &= phases[static_cast<std::size_t>(noctis::TravelPhase::parking)];

    const auto system =
        derive_planet_system(balastrackonastreya.x, balastrackonastreya.y, balastrackonastreya.z, 0, star_radius);
    constexpr std::int16_t felysia_index = 3;
    const auto &felysia                  = system.bodies[felysia_index];
    ok &= system.planet_count == 5 && system.body_count == 22;
    ok &= felysia.type == 3 && felysia.owner == -1;
    const auto local_target             = felysia_position(felysia, balastrackonastreya);
    auto local_guidance                 = noctis::begin_travel(ship, local_target);
    const double local_initial_distance = local_guidance.initial_distance;
    int local_ticks                     = 0;
    for (; local_ticks < 200000; ++local_ticks) {
        const auto step = noctis::advance_local_travel(ship, local_target, felysia.radius, local_guidance);
        phases[static_cast<std::size_t>(step.phase)] = true;
        consume_power(step.power_cost);
        if (step.arrived) {
            break;
        }
    }
    ok &= local_ticks == 392;
    ok &= local_initial_distance > 1.0;
    ok &= noctis::travel_distance(ship, local_target) < 2.0 * felysia.radius;
    ok &= phases[static_cast<std::size_t>(noctis::TravelPhase::approach)];
    ok &= charge == 117 && power == 19788;

    if (!ok) {
        std::fprintf(stderr,
                     "travel fixture failed: remote_ticks=%d remote_distance=%.17g "
                     "local_ticks=%d local_distance=%.17g local_initial=%.17g power=%d charge=%d\n",
                     remote_ticks, remote_arrival_distance, local_ticks, noctis::travel_distance(ship, local_target),
                     local_initial_distance, power, charge);
        return 1;
    }

    std::printf("BALASTRACKONASTREYA/FELYSIA journey: remote=%d ticks, local=%d ticks\n", remote_ticks, local_ticks);
    return 0;
}
