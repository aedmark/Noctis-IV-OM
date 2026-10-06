#include "engine_state.h"
#include "native_save.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace {
bool require(bool condition, const char *message) {
    if (!condition)
        std::fprintf(stderr, "Engine state: %s\n", message);
    return condition;
}
} // namespace

int main() {
    using namespace noctis;
    bool ok = true;

    reset_engine_state();
    auto &state = engine_state();
    ok &= require(state.travel.phase == TravelPhase::arrived, "default travel phase mismatch");
    ok &= require(state.travel.normalized_speed == 0.0F, "default travel speed mismatch");

    state.travel.begin(TravelPhase::charging);
    ok &= require(state.travel.phase == TravelPhase::charging, "travel begin phase mismatch");
    ok &= require(state.travel.normalized_speed == 0.0F, "travel begin did not clear speed");

    state.travel.update(TravelPhase::driving, 1.5F);
    ok &= require(state.travel.phase == TravelPhase::driving, "travel update phase mismatch");
    ok &= require(state.travel.normalized_speed == 1.0F, "travel speed upper clamp mismatch");

    state.travel.update(TravelPhase::parking, -0.5F);
    ok &= require(state.travel.normalized_speed == 0.0F, "travel speed lower clamp mismatch");

    reset_engine_state();
    ok &= require(state.travel.phase == TravelPhase::arrived, "travel reset phase mismatch");
    ok &= require(state.travel.normalized_speed == 0.0F, "travel reset speed mismatch");

    auto &application = state.application;
    ok &= require(application.mode == ApplicationMode::cockpit, "default application mode mismatch");
    ok &= require(application.transition_count == 0, "default transition count mismatch");
    ok &= require(application.transition_to(ApplicationMode::descent), "cockpit to descent rejected");
    ok &= require(application.transition_to(ApplicationMode::surface), "descent to surface rejected");
    ok &= require(application.transition_to(ApplicationMode::descent), "surface to ascent rejected");
    ok &= require(application.transition_to(ApplicationMode::cockpit), "ascent to cockpit rejected");
    ok &= require(application.transition_count == 4, "surface journey transition count mismatch");

    ok &= require(application.transition_to(ApplicationMode::gallery), "cockpit to gallery rejected");
    ok &= require(!application.transition_to(ApplicationMode::movie_player), "gallery to movie accepted");
    ok &= require(application.mode == ApplicationMode::gallery, "rejected transition changed mode");
    ok &= require(application.transition_count == 5, "rejected transition changed count");
    ok &= require(application.transition_to(ApplicationMode::cockpit), "gallery to cockpit rejected");
    ok &= require(application.transition_to(ApplicationMode::movie_player), "cockpit to movie rejected");
    ok &= require(application.transition_to(ApplicationMode::cockpit), "movie to cockpit rejected");

    ok &= require(application.transition_to(ApplicationMode::shutting_down), "shutdown transition rejected");
    ok &= require(!application.transition_to(ApplicationMode::cockpit), "transition out of shutdown accepted");
    ok &= require(application.mode == ApplicationMode::shutting_down, "shutdown mode changed after rejection");

    reset_engine_state();
    ok &= require(application.mode == ApplicationMode::cockpit, "application reset mode mismatch");
    ok &= require(application.transition_count == 0, "application reset count mismatch");

    NativeSaveState save;
    save.gnc_pos = 7;
    save.goesfile_pos = 123456;
    save.goesnet_command.fill(0);
    constexpr char command[] = "CAST FELYSIA";
    std::copy(std::begin(command), std::end(command), save.goesnet_command.begin());
    restore_goes_terminal_state(save, state.goes_terminal);
    ok &= require(state.goes_terminal.command_cursor == 7, "terminal cursor restore mismatch");
    ok &= require(state.goes_terminal.scroll_offset == 123456, "terminal scroll restore mismatch");
    ok &= require(state.goes_terminal.command == save.goesnet_command, "terminal command restore mismatch");

    NativeSaveState recaptured;
    capture_goes_terminal_state(state.goes_terminal, recaptured);
    ok &= require(recaptured.gnc_pos == save.gnc_pos, "terminal cursor capture mismatch");
    ok &= require(recaptured.goesfile_pos == save.goesfile_pos, "terminal scroll capture mismatch");
    ok &= require(recaptured.goesnet_command == save.goesnet_command, "terminal command capture mismatch");

    reset_engine_state();
    ok &= require(state.goes_terminal.command_cursor == 0, "terminal reset cursor mismatch");
    ok &= require(state.goes_terminal.scroll_offset == 0, "terminal reset scroll mismatch");
    ok &= require(state.goes_terminal.command.front() == '_' && state.goes_terminal.command[1] == 0,
                  "terminal reset command mismatch");
    return ok ? 0 : 1;
}
