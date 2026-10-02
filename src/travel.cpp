#include "travel.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace noctis {
namespace {

std::int16_t power_cost(double amount) {
    const auto bounded = std::clamp(amount, 0.0, static_cast<double>(std::numeric_limits<std::int16_t>::max()));
    return static_cast<std::int16_t>(bounded);
}

void move_remote(TravelPosition &position, const TravelPosition &target, double coefficient) {
    position.x -= (position.x - target.x) / coefficient;
    position.y -= (position.y - target.y) / coefficient;
    position.z -= (position.z - target.z) / coefficient;
}

void move_local(TravelPosition &position, const TravelPosition &target, double coefficient) {
    position.x -= (position.x - target.x) / coefficient;
    position.y -= (position.y - target.y) / (0.5 * coefficient);
    position.z -= (position.z - target.z) / coefficient;
}

void apply_guidance(TravelGuidance &guidance) {
    guidance.current_coefficient +=
        (guidance.requested_coefficient - guidance.current_coefficient) * guidance.reaction_time;
    guidance.current_coefficient = std::max(guidance.current_coefficient, 10.0);
}

} // namespace

double travel_distance(const TravelPosition &position, const TravelPosition &target) {
    const double dx = position.x - target.x;
    const double dy = position.y - target.y;
    const double dz = position.z - target.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool remote_target_in_range(const TravelPosition &target) {
    return std::sqrt(target.x * target.x + target.z * target.z) + std::abs(target.y * 30.0) <= 2.0E9;
}

TravelGuidance begin_travel(const TravelPosition &position, const TravelPosition &target) {
    TravelGuidance guidance;
    guidance.initial_distance      = travel_distance(position, target);
    guidance.requested_coefficient = 1000.0 * guidance.initial_distance;
    guidance.current_coefficient   = guidance.requested_coefficient;
    return guidance;
}

double remote_arrival_radius(bool direct_target, bool anti_radiation, double target_radius) {
    if (direct_target) {
        return 25000.0;
    }
    return (anti_radiation ? 44.0 : 1.5) * target_radius;
}

TravelStep advance_remote_travel(TravelPosition &position, const TravelPosition &target, double arrival_radius,
                                 TravelGuidance &guidance) {
    TravelStep step;
    step.distance = travel_distance(position, target);
    if (step.distance < arrival_radius) {
        step.phase   = TravelPhase::arrived;
        step.arrived = true;
        return step;
    }

    if (step.distance > 0.9999 * guidance.initial_distance) {
        guidance.requested_coefficient = 0.001 * step.distance;
        guidance.reaction_time         = 0.1;
        step.phase                     = TravelPhase::charging;
    } else if (step.distance < 7500.0 + arrival_radius) {
        guidance.requested_coefficient = 0.005 * step.distance;
        guidance.reaction_time         = 0.01;
        step.phase                     = TravelPhase::parking;
    } else if (step.distance < 15000.0 + arrival_radius) {
        guidance.requested_coefficient = 0.005 * step.distance;
        guidance.reaction_time         = 0.0025;
        step.phase                     = TravelPhase::linking;
    } else if (step.distance < 0.9990 * guidance.initial_distance) {
        guidance.requested_coefficient = 0.00001 * step.distance;
        guidance.reaction_time         = 0.05;
        step.phase                     = TravelPhase::driving;
    } else {
        guidance.requested_coefficient = 0.0002 * step.distance;
        guidance.reaction_time         = 0.08;
        step.phase                     = TravelPhase::ignition;
    }

    apply_guidance(guidance);
    move_remote(position, target, guidance.current_coefficient);
    step.power_cost = power_cost(step.distance * 1.0E-5);
    return step;
}

TravelStep advance_local_travel(TravelPosition &position, const TravelPosition &target, double target_radius,
                                TravelGuidance &guidance) {
    TravelStep step;
    step.distance = travel_distance(position, target);

    if (step.distance > 0.99999 * guidance.initial_distance) {
        guidance.requested_coefficient = 25.0 * step.distance;
        guidance.reaction_time         = 0.001;
        step.phase                     = TravelPhase::warming_up;
    } else if (step.distance < 25.0 && guidance.initial_distance > 500.0) {
        guidance.requested_coefficient = 50.0 * step.distance;
        guidance.reaction_time         = 0.0002;
        step.phase                     = TravelPhase::refining;
    } else if (step.distance < 100.0 && guidance.initial_distance > 500.0) {
        guidance.requested_coefficient = 15.0 * step.distance;
        guidance.reaction_time         = 0.0003;
        step.phase                     = TravelPhase::braking;
    } else if (step.distance < 0.995 * guidance.initial_distance) {
        guidance.requested_coefficient = 0.05 * step.distance;
        guidance.reaction_time         = 0.025;
        step.phase                     = TravelPhase::approach;
    } else {
        guidance.requested_coefficient = 1.5 * step.distance;
        guidance.reaction_time         = 0.05;
        step.phase                     = TravelPhase::ignition;
    }

    apply_guidance(guidance);
    move_local(position, target, guidance.current_coefficient);
    step.power_cost = power_cost(step.distance * 0.5E-5);
    if (step.distance < 2.0 * target_radius) {
        step.arrived = true;
    }
    return step;
}

const char *travel_phase_status(TravelPhase phase) {
    switch (phase) {
    case TravelPhase::charging:
        return "CHARGING";
    case TravelPhase::ignition:
        return "IGNITION";
    case TravelPhase::driving:
        return "DRIVING";
    case TravelPhase::linking:
        return "LINKING";
    case TravelPhase::parking:
        return "PARKING";
    case TravelPhase::warming_up:
        return "WARMING UP";
    case TravelPhase::approach:
        return "APPROACH";
    case TravelPhase::braking:
        return "BREAKING";
    case TravelPhase::refining:
        return "REFINING";
    case TravelPhase::arrived:
        return "STANDBY";
    }
    return "STANDBY";
}

} // namespace noctis
