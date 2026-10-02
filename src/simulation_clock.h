#pragma once

#include <cstdint>

namespace noctis {

inline constexpr std::int64_t simulation_tick_milliseconds = 55;

class SimulationClock {
public:
    void reset(double seconds);
    void advance(std::uint64_t count = 1);

    [[nodiscard]] double seconds() const;
    [[nodiscard]] double fraction() const;
    [[nodiscard]] std::uint64_t ticks() const;

private:
    double base_seconds_ = 0.0;
    std::uint64_t ticks_ = 0;
};

double current_universe_seconds();

// Timewarp simulation rate controls
int get_timewarp_multiplier();
void set_timewarp_multiplier(int multiplier);
int step_timewarp_multiplier(int step);
bool is_timewarp_active();
void set_timewarp_active(bool active);
void toggle_timewarp();

// Slider mapping helpers: maps log scale fraction [0.0, 1.0] <-> multiplier [1, 5000]
float timewarp_fraction_from_multiplier(int multiplier);
int timewarp_multiplier_from_fraction(float fraction);

} // namespace noctis
