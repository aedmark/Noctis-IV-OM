#include "ship_interface.h"

#include <array>
#include <cstdio>
#include <string>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "ship interface: %s\n", message);
    }
    return condition;
}
}

int main() {
    bool ok = true;

    constexpr std::array<std::int16_t, 10> keys{'5', 'r', 'p', 'x', '6', '7', '8', '9', '+', '-'};
    constexpr std::array<noctis::CockpitActionKind, 10> kinds{
        noctis::CockpitActionKind::select_menu, noctis::CockpitActionKind::select_menu,
        noctis::CockpitActionKind::select_menu, noctis::CockpitActionKind::select_menu,
        noctis::CockpitActionKind::run_command, noctis::CockpitActionKind::run_command,
        noctis::CockpitActionKind::run_command, noctis::CockpitActionKind::run_command,
        noctis::CockpitActionKind::brighten, noctis::CockpitActionKind::dim};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        const auto action = noctis::cockpit_action_for_key(keys[index]);
        ok &= require(action.kind == kinds[index], "cockpit shortcut kind mismatch");
        if (index < 8) {
            ok &= require(action.value == static_cast<std::int8_t>(index % 4 + 1),
                          "cockpit shortcut value mismatch");
        }
    }
    ok &= require(noctis::cockpit_action_for_key('q').kind == noctis::CockpitActionKind::none,
                  "unknown shortcut was accepted");
    ok &= require(noctis::space_snapshot_shortcut('m', false)
                      && !noctis::space_snapshot_shortcut('m', true)
                      && noctis::space_snapshot_shortcut('*', true),
                  "label-safe snapshot alias mismatch");
    ok &= require(noctis::cockpit_action_for_key('s', true).kind == noctis::CockpitActionKind::none
                      && noctis::cockpit_action_for_key('p', true).kind == noctis::CockpitActionKind::none,
                  "label text activated a ship shortcut");
    ok &= require(noctis::onboard_text_color('A') == 127
                      && noctis::onboard_text_color('a') == 127
                      && noctis::onboard_text_color('7') == 127,
                  "onboard text brightness is not stable");

    using P = noctis::ShipPreference;
    ok &= require(std::string(noctis::ship_preference_label(P::automatic_screen_sleep, false)) == "auto screen sleep off",
                  "screen-sleep label mismatch");
    ok &= require(std::string(noctis::ship_preference_label(P::reversed_pitch, true)) == "reverse pitch controls",
                  "pitch label mismatch");
    ok &= require(std::string(noctis::ship_preference_label(P::persistent_menus, true)) == "menus always onscreen",
                  "menu label mismatch");
    ok &= require(std::string(noctis::ship_preference_label(P::polarized_hull, false)) == "depolarize",
                  "hull label mismatch");

    const auto epoc_6011_end = noctis::split_triad_time(999'999'999.0);
    ok &= require(epoc_6011_end.sinister == 999 && epoc_6011_end.medius == 999
                      && epoc_6011_end.dexter == 999,
                  "Epoc 6011 final triad mismatch");
    const auto epoc_6012_start = noctis::split_triad_time(1'000'000'000.0);
    ok &= require(epoc_6012_start.sinister == 0 && epoc_6012_start.medius == 0
                      && epoc_6012_start.dexter == 0,
                  "Epoc 6012 sinister did not wrap to zero");
    const auto padded = noctis::split_triad_time(1'007'008'009.0);
    ok &= require(noctis::format_triad(padded.sinister) == "007"
                      && noctis::format_triad(padded.medius) == "008"
                      && noctis::format_triad(padded.dexter) == "009",
                  "triad components were not padded to three digits");

    std::FILE *stream = std::tmpfile();
    ok &= require(stream != nullptr, "could not create temporary GOES output");
    if (stream != nullptr) {
        std::array<std::uint8_t, 400> source{};
        for (std::size_t index = 0; index < source.size(); ++index) {
            source[index] = static_cast<std::uint8_t>('A' + index % 26);
        }
        ok &= require(std::fwrite(source.data(), 1, source.size(), stream) == source.size(),
                      "could not populate GOES output");
        std::fflush(stream);
        ok &= require(noctis::stream_length(stream) == 400, "GOES output length mismatch");
        ok &= require(noctis::goes_scroll_offset(0, 400, noctis::GoesScroll::end) == 253,
                      "End did not select the final full screen");
        ok &= require(noctis::goes_scroll_offset(0, 400, noctis::GoesScroll::line_up) == 0,
                      "line-up underflow was not clamped");
        ok &= require(noctis::goes_scroll_offset(253, 400, noctis::GoesScroll::page_down) == 253,
                      "page-down overflow was not clamped");
        ok &= require(noctis::goes_scroll_offset(147, 400, noctis::GoesScroll::page_up) == 0,
                      "page-up mismatch");

        std::array<std::uint8_t, noctis::goes_screen_bytes + 1> screen{};
        const auto count = noctis::read_goes_screen(stream, 253, screen);
        ok &= require(count == noctis::goes_screen_bytes, "full GOES page was not read byte-for-byte");
        ok &= require(screen.front() == source[253] && screen[146] == source[399] && screen[147] == 0,
                      "GOES page content or terminator mismatch");
        std::fclose(stream);
    }

    return ok ? 0 : 1;
}
