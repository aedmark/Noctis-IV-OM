#include "engine_state.h"

#include <cstdio>

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
    return ok ? 0 : 1;
}
