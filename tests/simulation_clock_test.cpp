#include "simulation_clock.h"

#include <cmath>
#include <cstdio>

namespace {
bool expect_near(double actual, double expected, const char *name) {
    if (std::abs(actual - expected) <= 1e-9) {
        return true;
    }
    std::fprintf(stderr, "%s: expected %.12f, got %.12f\n", name, expected, actual);
    return false;
}
}

int main() {
    noctis::SimulationClock clock;
    clock.reset(1000.25);

    bool ok = true;
    ok &= expect_near(clock.seconds(), 1000.25, "reset seconds");
    ok &= expect_near(clock.fraction(), 0.25, "reset fraction");

    clock.advance();
    ok &= expect_near(clock.seconds(), 1000.305, "one fixed tick");
    ok &= clock.ticks() == 1;

    for (int i = 1; i < 1000; ++i) {
        clock.advance();
    }
    ok &= expect_near(clock.seconds(), 1055.25, "one thousand fixed ticks");
    ok &= expect_near(clock.fraction(), 0.25, "stable long-run fraction");

    // Reset models a save/startup wall-clock synchronization. Subsequent
    // simulation time is again an exact function of the number of ticks.
    clock.reset(42.0);
    for (int i = 0; i < 18; ++i) {
        clock.advance();
    }
    ok &= expect_near(clock.seconds(), 42.99, "reset then eighteen ticks");
    ok &= clock.ticks() == 18;

    // Test advance with arbitrary count (timewarp)
    clock.reset(0.0);
    clock.advance(100);
    ok &= expect_near(clock.seconds(), 5.5, "advance 100 ticks (5.5 seconds)");
    ok &= clock.ticks() == 100;

    // Test timewarp multiplier controls
    noctis::set_timewarp_multiplier(100);
    ok &= noctis::get_timewarp_multiplier() == 100;
    ok &= !noctis::is_timewarp_active();
    noctis::toggle_timewarp();
    ok &= noctis::is_timewarp_active();
    noctis::toggle_timewarp();
    ok &= !noctis::is_timewarp_active();

    // Step up and down
    noctis::set_timewarp_multiplier(100);
    ok &= noctis::step_timewarp_multiplier(1) == 250;
    ok &= noctis::step_timewarp_multiplier(-1) == 100;
    ok &= noctis::step_timewarp_multiplier(-1) == 50;

    // Slider fraction round-trip
    const float f500 = noctis::timewarp_fraction_from_multiplier(500);
    ok &= noctis::timewarp_multiplier_from_fraction(f500) == 500;
    ok &= noctis::timewarp_multiplier_from_fraction(0.0f) == 1;
    ok &= noctis::timewarp_multiplier_from_fraction(1.0f) == 5000;

    return ok ? 0 : 1;
}
