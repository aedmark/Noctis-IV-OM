#include "plus_presentation.h"

#include <cstdio>

namespace noctis {

std::string format_radius(float centidyams) {
    char text[48];
    std::snprintf(text, sizeof(text), "RADIUS: %1.4f CENTIDYAMS", centidyams);
    return text;
}

std::int8_t cycle_lens_flare_mode(std::int8_t mode) {
    return mode == 0 ? 1 : mode == 1 ? -1 : 0;
}

bool lens_flare_on_hud(bool call_requests_hud_flare, std::int8_t mode) {
    return mode == 1 || (mode == 0 && call_requests_hud_flare);
}

std::uint16_t visible_surface_objects(std::uint16_t count, std::int32_t depth) {
    return depth > 255 ? static_cast<std::uint16_t>(count >> 1U) : count;
}

const std::vector<std::string_view> &plus_help_lines(bool surface) {
    static const std::vector<std::string_view> space{
        "NOCTIS IV PLUS - SPACE SHORTCUTS",
        "M/* SNAPSHOT   B/DELETE RAW SNAPSHOT",
        "S ROOFSPEED   DOWN MOUSELOOK",
        "F2 VISUAL EFFECTS   F3 MOVIEMAKER"};
    static const std::vector<std::string_view> ground{
        "NOCTIS IV PLUS - SURFACE SHORTCUTS",
        "J JUMP   SPACE JETPACK   C RELEASE",
        "M/* SNAP   N// WIDE   B/DEL RAW   V/. RAW WIDE",
        "DOWN MOUSELOOK   F2 EFFECTS   F3 MOVIE"};
    return surface ? ground : space;
}

const std::vector<std::string> plus_visual_menu_lines(bool draw_hud,
                                                       std::int8_t lens_flare_mode,
                                                       bool seamless_border) {
    return {"NOCTIS IV+ VISUAL EFFECTS SETTINGS",
            draw_hud ? "HUD TEXT ON (T)" : "HUD TEXT OFF (T)",
            lens_flare_mode == 1 ? "LENS FLARES ALWAYS ON (F)"
                : lens_flare_mode == -1 ? "LENS FLARES ALWAYS OFF (F)"
                                        : "VISOR LENS FLARES ONLY (F)",
            seamless_border ? "SEAMLESS BORDER (B)" : "DEFAULT BORDER (B)"};
}

const std::vector<std::string> plus_movie_menu_lines(std::uint16_t deck,
                                                      std::uint16_t cadence,
                                                      bool black_flash,
                                                      bool occupied,
                                                      bool recording,
                                                      bool paused,
                                                      double captured_fps) {
    char deck_line[48];
    char cadence_line[48];
    char action_line[48];
    std::snprintf(deck_line, sizeof(deck_line), "MOVIEDECK %03u%s (CTRL +/-)", deck,
                  occupied && !recording && !paused ? " EXISTS" : "");
    std::snprintf(cadence_line, sizeof(cadence_line), "CAPTURE EVERY %03u FRAMES (+/-)", cadence);
    if (recording) std::snprintf(action_line, sizeof(action_line), "STOP RECORDING - FPS %05.2f (ENTER)", captured_fps);
    else if (paused) std::snprintf(action_line, sizeof(action_line), "RESUME RECORDING (P/ENTER)");
    else std::snprintf(action_line, sizeof(action_line), "START RECORDING (ENTER)");
    return {"NOCTIS IV+ MOVIEMAKER", deck_line, cadence_line,
            black_flash ? "BLACK FLASH WHEN CAPTURING (F)" : "NO BLACK FLASH WHEN CAPTURING (F)",
            action_line};
}

} // namespace noctis
