#pragma once

#include <cstdint>

namespace noctis {

inline constexpr std::int64_t simulation_tick_milliseconds = 55;

class SimulationClock {
public:
    void reset(double seconds);
    void advance();

    [[nodiscard]] double seconds() const;
    [[nodiscard]] double fraction() const;
    [[nodiscard]] std::uint64_t ticks() const;

private:
    double base_seconds_ = 0.0;
    std::uint64_t ticks_ = 0;
};

double current_universe_seconds();

} // namespace noctis
