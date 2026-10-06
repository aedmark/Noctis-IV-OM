#include "engine_state.h"

#include "native_save.h"

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

void GoesTerminalState::reset() noexcept {
    command_cursor = 0;
    scroll_offset  = 0;
    command.fill(0);
    command.front() = '_';
}

void capture_goes_terminal_state(const GoesTerminalState &terminal, NativeSaveState &save) noexcept {
    save.gnc_pos         = terminal.command_cursor;
    save.goesfile_pos    = terminal.scroll_offset;
    save.goesnet_command = terminal.command;
}

void restore_goes_terminal_state(const NativeSaveState &save, GoesTerminalState &terminal) noexcept {
    terminal.command_cursor = save.gnc_pos;
    terminal.scroll_offset  = save.goesfile_pos;
    terminal.command        = save.goesnet_command;
}

void EngineState::reset() noexcept {
    travel.reset();
    application.reset();
    goes_terminal.reset();
}

EngineState &engine_state() noexcept {
    static EngineState state;
    return state;
}

void reset_engine_state() noexcept { engine_state().reset(); }

} // namespace noctis
