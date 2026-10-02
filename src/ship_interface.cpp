#include "ship_interface.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace noctis {

std::int64_t stream_length(std::FILE *stream) {
    if (stream == nullptr) {
        return -1;
    }
    const long original = std::ftell(stream);
    if (original < 0 || std::fseek(stream, 0, SEEK_END) != 0) {
        return -1;
    }
    const long length = std::ftell(stream);
    if (std::fseek(stream, original, SEEK_SET) != 0) {
        return -1;
    }
    return length;
}

std::int32_t goes_scroll_offset(std::int32_t current, std::int64_t file_size, GoesScroll scroll) {
    const std::int64_t last = std::max<std::int64_t>(0, file_size - static_cast<std::int64_t>(goes_screen_bytes));
    std::int64_t next = current;
    switch (scroll) {
    case GoesScroll::home: next = 0; break;
    case GoesScroll::end: next = last; break;
    case GoesScroll::line_up: next -= goes_screen_columns; break;
    case GoesScroll::line_down: next += goes_screen_columns; break;
    case GoesScroll::page_up: next -= goes_screen_bytes; break;
    case GoesScroll::page_down: next += goes_screen_bytes; break;
    case GoesScroll::none: break;
    }
    next = std::clamp<std::int64_t>(next, 0, last);
    return static_cast<std::int32_t>(std::min<std::int64_t>(next, std::numeric_limits<std::int32_t>::max()));
}

std::size_t read_goes_screen(std::FILE *stream, std::int32_t offset,
                             std::array<std::uint8_t, goes_screen_bytes + 1> &screen) {
    screen.fill(0);
    const auto length = stream_length(stream);
    if (length < 0) {
        return 0;
    }
    const auto bounded = goes_scroll_offset(offset, length, GoesScroll::none);
    if (std::fseek(stream, bounded, SEEK_SET) != 0) {
        return 0;
    }
    return std::fread(screen.data(), 1, goes_screen_bytes, stream);
}

CockpitAction cockpit_action_for_key(std::int16_t key, bool label_entry) {
    if (label_entry) return {};
    switch (key) {
    case '5': return {CockpitActionKind::select_menu, 1};
    case 'r': return {CockpitActionKind::select_menu, 2};
    case 'p': return {CockpitActionKind::select_menu, 3};
    case 'x': return {CockpitActionKind::select_menu, 4};
    case '6': return {CockpitActionKind::run_command, 1};
    case '7': return {CockpitActionKind::run_command, 2};
    case '8': return {CockpitActionKind::run_command, 3};
    case '9': return {CockpitActionKind::run_command, 4};
    case '+': return {CockpitActionKind::brighten, 0};
    case '-': return {CockpitActionKind::dim, 0};
    default: return {};
    }
}

std::uint8_t onboard_text_color(std::uint8_t) {
    // The inherited renderer pulsed uppercase text through six brightness
    // levels every simulation tick. At native refresh rates that makes whole
    // menu headings appear to flicker, so keep all cockpit text stable.
    return 127;
}

bool space_snapshot_shortcut(std::int16_t key, bool label_entry) {
    return key == '*' || (key == 'm' && !label_entry);
}

const char *ship_preference_label(ShipPreference preference, bool enabled) {
    switch (preference) {
    case ShipPreference::automatic_screen_sleep:
        return enabled ? "auto screen sleep on" : "auto screen sleep off";
    case ShipPreference::reversed_pitch:
        return enabled ? "reverse pitch controls" : "normal pitch controls";
    case ShipPreference::persistent_menus:
        return enabled ? "menus always onscreen" : "auto-hidden menus";
    case ShipPreference::polarized_hull:
        return enabled ? "polarize" : "depolarize";
    }
    return "";
}

TriadTime split_triad_time(double seconds) {
    if (!std::isfinite(seconds)) return {};
    double within_epoc = std::fmod(seconds, 1'000'000'000.0);
    if (within_epoc < 0) within_epoc += 1'000'000'000.0;
    const auto whole = static_cast<std::uint32_t>(within_epoc);
    return {static_cast<std::uint16_t>((whole / 1'000'000U) % 1'000U),
            static_cast<std::uint16_t>((whole / 1'000U) % 1'000U),
            static_cast<std::uint16_t>(whole % 1'000U)};
}

std::string format_triad(std::uint16_t value) {
    char text[4];
    std::snprintf(text, sizeof(text), "%03u", static_cast<unsigned>(value % 1'000U));
    return text;
}

} // namespace noctis
