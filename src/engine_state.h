#pragma once

#include "travel.h"

namespace noctis {

// Transient runtime state owned by the application rather than by an
// individual legacy translation unit. Persistent navigation values remain in
// NativeSaveState until their compatibility boundaries are extracted.
struct TravelRuntimeState {
    TravelPhase phase      = TravelPhase::arrived;
    float normalized_speed = 0.0F;

    void begin(TravelPhase initial_phase) noexcept;
    void update(TravelPhase current_phase, float speed) noexcept;
    void reset() noexcept;
};

struct EngineState {
    TravelRuntimeState travel;

    void reset() noexcept;
};

// Single application-owned state root. New engine state should be added here
// and passed to extracted systems instead of introducing more extern globals.
EngineState &engine_state() noexcept;
void reset_engine_state() noexcept;

} // namespace noctis
