#include "plus_presentation.h"

#include <algorithm>
#include <cmath>
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

void apply_suit_torch(std::uint8_t *framebuffer, std::int32_t width, std::int32_t height, bool torch_active) {
    if (!torch_active || !framebuffer || width <= 0 || height <= 0) return;

    const int cx = width / 2;
    const int cy = height / 2 + 8;
    constexpr float rx = 85.0f;
    constexpr float ry = 65.0f;
    constexpr float inv_rx2 = 1.0f / (rx * rx);
    constexpr float inv_ry2 = 1.0f / (ry * ry);

    const int min_y = std::max(0, cy - static_cast<int>(ry));
    const int max_y = std::min(height - 1, cy + static_cast<int>(ry));
    const int min_x = std::max(0, cx - static_cast<int>(rx));
    const int max_x = std::min(width - 1, cx + static_cast<int>(rx));

    for (int y = min_y; y <= max_y; ++y) {
        const float dy = static_cast<float>(y - cy);
        const float dy2_term = dy * dy * inv_ry2;
        if (dy2_term >= 1.0f) continue;

        auto *row = framebuffer + y * width;
        for (int x = min_x; x <= max_x; ++x) {
            const float dx = static_cast<float>(x - cx);
            const float d2 = dx * dx * inv_rx2 + dy2_term;
            if (d2 >= 1.0f) continue;

            const float edge_fade = 1.0f - d2;
            float intensity = edge_fade * edge_fade;
            if (d2 < 0.20f) {
                intensity += (1.0f - d2 / 0.20f) * 0.35f;
            }

            constexpr float max_boost = 32.0f;
            const int boost = static_cast<int>(intensity * max_boost);
            if (boost <= 0) continue;

            std::uint8_t &val = row[x];
            if (val < 44) {
                val = static_cast<std::uint8_t>(std::min(43, val + boost));
            } else if (val < 64) {
                val = static_cast<std::uint8_t>(std::min(63, val + (boost >> 1)));
            } else if (val >= 128 && val < 138) {
                val = static_cast<std::uint8_t>(std::min(137, val + (boost >> 2)));
            } else if (val >= 192) {
                val = static_cast<std::uint8_t>(std::min(255, val + boost));
            }
        }
    }
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
        "L TORCH   M/* SNAP   N// WIDE   B/DEL RAW",
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
