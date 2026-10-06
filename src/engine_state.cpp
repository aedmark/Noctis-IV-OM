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

void EngineState::reset() noexcept { travel.reset(); }

EngineState &engine_state() noexcept {
    static EngineState state;
    return state;
}

void reset_engine_state() noexcept { engine_state().reset(); }

} // namespace noctis
