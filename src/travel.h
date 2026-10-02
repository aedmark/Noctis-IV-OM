#pragma once

#include <cstdint>

namespace noctis {

struct TravelPosition {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

enum class TravelPhase {
    charging,
    ignition,
    driving,
    linking,
    parking,
    warming_up,
    approach,
    braking,
    refining,
    arrived,
};

struct TravelGuidance {
    double initial_distance      = 0.0;
    double requested_coefficient = 1.0;
    double current_coefficient   = 1.0;
    double reaction_time         = 0.01;
};

struct TravelStep {
    TravelPhase phase       = TravelPhase::arrived;
    double distance         = 0.0;
    std::int16_t power_cost = 0;
    bool arrived            = false;
};

[[nodiscard]] double travel_distance(const TravelPosition &position, const TravelPosition &target);
[[nodiscard]] bool remote_target_in_range(const TravelPosition &target);
[[nodiscard]] TravelGuidance begin_travel(const TravelPosition &position, const TravelPosition &target);
[[nodiscard]] double remote_arrival_radius(bool direct_target, bool anti_radiation, double target_radius);
[[nodiscard]] TravelStep advance_remote_travel(TravelPosition &position, const TravelPosition &target,
                                               double arrival_radius, TravelGuidance &guidance);
[[nodiscard]] TravelStep advance_local_travel(TravelPosition &position, const TravelPosition &target,
                                              double target_radius, TravelGuidance &guidance);
[[nodiscard]] const char *travel_phase_status(TravelPhase phase);

} // namespace noctis
