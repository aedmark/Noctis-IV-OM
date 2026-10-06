#pragma once

#include "travel.h"

#include <array>
#include <cstdint>

namespace noctis {

struct NativeSaveState;

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

enum class ApplicationMode : std::uint8_t {
    cockpit,
    descent,
    surface,
    gallery,
    movie_player,
    shutting_down,
};

[[nodiscard]] bool can_transition(ApplicationMode from, ApplicationMode to) noexcept;

struct ApplicationRuntimeState {
    ApplicationMode mode = ApplicationMode::cockpit;
    std::uint64_t transition_count = 0;

    [[nodiscard]] bool transition_to(ApplicationMode next) noexcept;
    void reset() noexcept;
};

struct GoesTerminalState {
    std::int8_t command_cursor = 0;
    std::int32_t scroll_offset = 0;
    std::array<char, 120> command{'_'};

    void reset() noexcept;
};

// Persistence adapters deliberately keep NativeSaveState's v1 schema flat.
// Runtime ownership can evolve without changing either native or legacy bytes.
void capture_goes_terminal_state(const GoesTerminalState &terminal, NativeSaveState &save) noexcept;
void restore_goes_terminal_state(const NativeSaveState &save, GoesTerminalState &terminal) noexcept;

struct EngineState {
    TravelRuntimeState travel;
    ApplicationRuntimeState application;
    GoesTerminalState goes_terminal;

    void reset() noexcept;
};

// Single application-owned state root. New engine state should be added here
// and passed to extracted systems instead of introducing more extern globals.
EngineState &engine_state() noexcept;
void reset_engine_state() noexcept;

} // namespace noctis
