#include "simulation_clock.h"

#include <chrono>
#include <cmath>
#include <ctime>

namespace noctis {

void SimulationClock::reset(double seconds) {
    base_seconds_ = seconds;
    ticks_ = 0;
}

void SimulationClock::advance() { ++ticks_; }

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

} // namespace noctis
