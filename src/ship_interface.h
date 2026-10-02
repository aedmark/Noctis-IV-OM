#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace noctis {

constexpr std::size_t goes_screen_columns = 21;
constexpr std::size_t goes_screen_rows = 7;
constexpr std::size_t goes_screen_bytes = goes_screen_columns * goes_screen_rows;

enum class GoesScroll {
    none,
    home,
    end,
    line_up,
    line_down,
    page_up,
    page_down,
};

std::int64_t stream_length(std::FILE *stream);
std::int32_t goes_scroll_offset(std::int32_t current, std::int64_t file_size, GoesScroll scroll);
std::size_t read_goes_screen(std::FILE *stream, std::int32_t offset,
                             std::array<std::uint8_t, goes_screen_bytes + 1> &screen);

enum class CockpitActionKind {
    none,
    select_menu,
    run_command,
    brighten,
    dim,
};

struct CockpitAction {
    CockpitActionKind kind = CockpitActionKind::none;
    std::int8_t value = 0;
};

CockpitAction cockpit_action_for_key(std::int16_t key, bool label_entry = false);
[[nodiscard]] std::uint8_t onboard_text_color(std::uint8_t character);
bool space_snapshot_shortcut(std::int16_t key, bool label_entry);

enum class ShipPreference : std::uint8_t {
    automatic_screen_sleep,
    reversed_pitch,
    persistent_menus,
    polarized_hull,
};

const char *ship_preference_label(ShipPreference preference, bool enabled);

struct TriadTime {
    std::uint16_t sinister{};
    std::uint16_t medius{};
    std::uint16_t dexter{};
};

TriadTime split_triad_time(double seconds);
std::string format_triad(std::uint16_t value);

} // namespace noctis
