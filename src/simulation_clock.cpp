#include "simulation_clock.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>

namespace noctis {

namespace {
constexpr std::array<int, 11> g_timewarp_steps = {1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 5000};
int g_timewarp_multiplier = 100;
bool g_timewarp_active = false;
}

void SimulationClock::reset(double seconds) {
    base_seconds_ = seconds;
    ticks_ = 0;
}

void SimulationClock::advance(std::uint64_t count) { ticks_ += count; }

double SimulationClock::seconds() const {
    constexpr double seconds_per_tick = static_cast<double>(simulation_tick_milliseconds) / 1000.0;
    return base_seconds_ + static_cast<double>(ticks_) * seconds_per_tick;
}

double SimulationClock::fraction() const {
    const double value = seconds();
    return value - std::floor(value);
}

std::uint64_t SimulationClock::ticks() const { return ticks_; }

double current_universe_seconds() {
    // Preserve the legacy port's local-time epoch and compatibility offset.
    std::tm epoch{};
    epoch.tm_mday = 1;
    epoch.tm_year = 84;

    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    return std::difftime(now_time, std::mktime(&epoch)) - 82800.0;
}

int get_timewarp_multiplier() { return g_timewarp_multiplier; }

void set_timewarp_multiplier(int multiplier) {
    g_timewarp_multiplier = std::clamp(multiplier, 1, 5000);
}

int step_timewarp_multiplier(int step) {
    int current_idx = 0;
    int min_diff = std::abs(g_timewarp_multiplier - g_timewarp_steps[0]);
    for (std::size_t i = 1; i < g_timewarp_steps.size(); ++i) {
        int diff = std::abs(g_timewarp_multiplier - g_timewarp_steps[i]);
        if (diff < min_diff) {
            min_diff = diff;
            current_idx = static_cast<int>(i);
        }
    }
    int new_idx = std::clamp(current_idx + step, 0, static_cast<int>(g_timewarp_steps.size()) - 1);
    g_timewarp_multiplier = g_timewarp_steps[new_idx];
    return g_timewarp_multiplier;
}

bool is_timewarp_active() { return g_timewarp_active; }

void set_timewarp_active(bool active) { g_timewarp_active = active; }

void toggle_timewarp() { g_timewarp_active = !g_timewarp_active; }

float timewarp_fraction_from_multiplier(int multiplier) {
    if (multiplier <= 1) return 0.0f;
    if (multiplier >= 5000) return 1.0f;
    return static_cast<float>(std::log10(multiplier) / std::log10(5000.0));
}

int timewarp_multiplier_from_fraction(float fraction) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    if (fraction <= 0.001f) return 1;
    if (fraction >= 0.999f) return 5000;
    const double raw = std::pow(10.0, fraction * std::log10(5000.0));
    int best_step = g_timewarp_steps[0];
    double best_diff = std::abs(raw - best_step);
    for (int step : g_timewarp_steps) {
        double diff = std::abs(raw - step);
        if (diff < best_diff) {
            best_diff = diff;
            best_step = step;
        }
    }
    return best_step;
}

} // namespace noctis
