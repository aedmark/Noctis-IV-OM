#include "engine_state.h"

#include <algorithm>

namespace noctis {

void TravelRuntimeState::begin(TravelPhase initial_phase) noexcept {
    phase            = initial_phase;
    normalized_speed = 0.0F;
}

void TravelRuntimeState::update(TravelPhase current_phase, float speed) noexcept {
    phase            = current_phase;
    normalized_speed = std::clamp(speed, 0.0F, 1.0F);
}

void TravelRuntimeState::reset() noexcept {
    phase            = TravelPhase::arrived;
    normalized_speed = 0.0F;
}

bool can_transition(ApplicationMode from, ApplicationMode to) noexcept {
    if (from == to) return true;
    if (to == ApplicationMode::shutting_down) return true;

    switch (from) {
    case ApplicationMode::cockpit:
        return to == ApplicationMode::descent || to == ApplicationMode::gallery ||
               to == ApplicationMode::movie_player;
    case ApplicationMode::descent:
        return to == ApplicationMode::surface || to == ApplicationMode::cockpit;
    case ApplicationMode::surface:
        return to == ApplicationMode::descent || to == ApplicationMode::cockpit;
    case ApplicationMode::gallery:
    case ApplicationMode::movie_player:
        return to == ApplicationMode::cockpit;
    case ApplicationMode::shutting_down:
        return false;
    }
    return false;
}

bool ApplicationRuntimeState::transition_to(ApplicationMode next) noexcept {
    if (!can_transition(mode, next)) return false;
    if (mode != next) {
        mode = next;
        ++transition_count;
    }
    return true;
}

void ApplicationRuntimeState::reset() noexcept {
    mode = ApplicationMode::cockpit;
    transition_count = 0;
}

void EngineState::reset() noexcept {
    travel.reset();
    application.reset();
}

EngineState &engine_state() noexcept {
    static EngineState state;
    return state;
}

void reset_engine_state() noexcept { engine_state().reset(); }

} // namespace noctis
