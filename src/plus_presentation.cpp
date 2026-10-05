#include "plus_presentation.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

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

    const float scale_x = static_cast<float>(width) / 320.0f;
    const float scale_y = static_cast<float>(height) / 200.0f;
    const int cx = width / 2;
    const int cy = height / 2 + static_cast<int>(std::round(8.0f * scale_y));
    const float rx = 85.0f * scale_x;
    const float ry = 65.0f * scale_y;
    const float inv_rx2 = 1.0f / (rx * rx);
    const float inv_ry2 = 1.0f / (ry * ry);

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

void apply_suit_torch_rgba(std::uint8_t *rgba, const std::uint8_t *adapted,
                           std::int32_t width, std::int32_t height) {
    if (!rgba || !adapted || width <= 0 || height <= 0) return;

    const float scale_x = static_cast<float>(width) / 320.0f;
    const float scale_y = static_cast<float>(height) / 200.0f;
    const int cx = width / 2;
    const int cy = height / 2 + static_cast<int>(std::round(10.0f * scale_y));
    const float rx = 96.0f * scale_x;
    const float ry = 70.0f * scale_y;
    const float inv_rx2 = 1.0f / (rx * rx);
    const float inv_ry2 = 1.0f / (ry * ry);

    const int margin_y = static_cast<int>(std::round(10.0f * scale_y));
    const int margin_x = static_cast<int>(std::round(10.0f * scale_x));
    const int min_y = std::max(margin_y, cy - static_cast<int>(ry));
    const int max_y = std::min(height - 1 - margin_y, cy + static_cast<int>(ry));
    const int min_x = std::max(margin_x, cx - static_cast<int>(rx));
    const int max_x = std::min(width - 1 - margin_x, cx + static_cast<int>(rx));

    for (int y = min_y; y <= max_y; ++y) {
        const float dy = static_cast<float>(y - cy);
        const float dy2_term = dy * dy * inv_ry2;
        if (dy2_term >= 1.0f) continue;

        const std::size_t row_pixel_offset = static_cast<std::size_t>(y) * width;
        for (int x = min_x; x <= max_x; ++x) {
            const float dx = static_cast<float>(x - cx);
            const float d2 = dx * dx * inv_rx2 + dy2_term;
            if (d2 >= 1.0f) continue;

            const std::uint8_t idx = adapted[row_pixel_offset + x];
            // Preserve sky, stars, menu overlays, and status text (64..127)
            if (idx >= 64 && idx <= 127) continue;
            // Preserve distant horizon sky haze (138..191)
            if (idx >= 138 && idx < 192) continue;

            const float edge_fade = 1.0f - d2;
            float intensity = edge_fade * edge_fade;
            if (d2 < 0.25f) {
                intensity += (1.0f - d2 / 0.25f) * 0.40f;
            }

            float surface_mod = 1.0f;
            if (idx < 44) {
                surface_mod = 0.70f + 0.30f * (static_cast<float>(idx) / 43.0f);
            } else if (idx < 64) {
                surface_mod = 1.0f;
            } else if (idx >= 192) {
                surface_mod = 0.75f + 0.25f * (static_cast<float>(idx - 192) / 63.0f);
            } else if (idx >= 128 && idx < 138) {
                surface_mod = 0.65f + 0.35f * (static_cast<float>(idx - 128) / 9.0f);
            }

            const std::size_t rgba_idx = (row_pixel_offset + x) * 4;
            const int cur_r = rgba[rgba_idx + 0];
            const int cur_g = rgba[rgba_idx + 1];
            const int cur_b = rgba[rgba_idx + 2];

            // True albedo luminance scaling: illuminates the surface's existing
            // colors and texture by increasing luminance, preserving hue and saturation
            // without washing out into a flat white circular overlay.
            const float lum = cur_r * 0.299f + cur_g * 0.587f + cur_b * 0.114f;
            const float ambient_scale = 1.0f - std::clamp((lum - 15.0f) / 165.0f, 0.0f, 0.85f);
            const float delta_lum = intensity * surface_mod * 145.0f * ambient_scale;

            float out_r_f, out_g_f, out_b_f;
            if (lum > 0.5f) {
                const float scale = 1.0f + delta_lum / lum;
                const float target_r = cur_r * scale;
                const float target_g = cur_g * scale;
                const float target_b = cur_b * scale;

                if (lum < 12.0f) {
                    const float w_color = lum / 12.0f;
                    const float r_diff = cur_r + delta_lum;
                    const float g_diff = cur_g + delta_lum * 0.96f;
                    const float b_diff = cur_b + delta_lum * 0.90f;
                    out_r_f = (1.0f - w_color) * r_diff + w_color * target_r;
                    out_g_f = (1.0f - w_color) * g_diff + w_color * target_g;
                    out_b_f = (1.0f - w_color) * b_diff + w_color * target_b;
                } else {
                    out_r_f = target_r;
                    out_g_f = target_g;
                    out_b_f = target_b;
                }
            } else {
                out_r_f = cur_r + delta_lum;
                out_g_f = cur_g + delta_lum * 0.96f;
                out_b_f = cur_b + delta_lum * 0.90f;
            }

            rgba[rgba_idx + 0] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(out_r_f + 0.5f), 0, 255));
            rgba[rgba_idx + 1] = static_cast<std::uint8_t>(static_cast<int>(std::clamp(static_cast<int>(out_g_f + 0.5f), 0, 255)));
            rgba[rgba_idx + 2] = static_cast<std::uint8_t>(static_cast<int>(std::clamp(static_cast<int>(out_b_f + 0.5f), 0, 255)));
        }
    }
}

namespace {
constexpr std::uint8_t surface_hud_font[65 * 5] = {
    0, 0, 0, 0, 0, // 32 ' '
    2, 2, 2, 0, 2, // 33 '!'
    5, 0, 0, 0, 0, // 34 '"'
    0, 0, 3, 5, 5, // 35 '#'
    2, 2, 6, 2, 2, // 36 '$'
    1, 4, 2, 1, 4, // 37 '%'
    0, 0, 2, 0, 0, // 38 '&'
    0, 2, 2, 0, 0, // 39 '\''
    4, 2, 2, 2, 4, // 40 '('
    1, 2, 2, 2, 1, // 41 ')'
    0, 0, 7, 2, 2, // 42 '*'
    0, 2, 7, 2, 0, // 43 '+'
    0, 0, 0, 2, 1, // 44 ','
    0, 0, 7, 0, 0, // 45 '-'
    0, 0, 0, 0, 2, // 46 '.'
    0, 4, 2, 1, 0, // 47 '/'
    7, 5, 5, 5, 7, // 48 '0'
    3, 2, 2, 2, 7, // 49 '1'
    7, 4, 7, 1, 7, // 50 '2'
    7, 4, 6, 4, 7, // 51 '3'
    4, 6, 5, 7, 4, // 52 '4'
    7, 1, 7, 4, 7, // 53 '5'
    7, 1, 7, 5, 7, // 54 '6'
    7, 4, 4, 4, 4, // 55 '7'
    7, 5, 7, 5, 7, // 56 '8'
    7, 5, 7, 4, 4, // 57 '9'
    0, 2, 0, 2, 0, // 58 ':'
    0, 2, 0, 2, 1, // 59 ';'
    4, 2, 1, 2, 4, // 60 '<'
    0, 7, 0, 7, 0, // 61 '='
    1, 2, 4, 2, 1, // 62 '>'
    7, 4, 6, 0, 2, // 63 '?'
    0, 2, 0, 0, 0, // 64 '@'
    7, 5, 7, 5, 5, // 65 'A'
    7, 5, 3, 5, 7, // 66 'B'
    7, 1, 1, 1, 7, // 67 'C'
    3, 5, 5, 5, 3, // 68 'D'
    7, 1, 3, 1, 7, // 69 'E'
    7, 1, 3, 1, 1, // 70 'F'
    7, 1, 5, 5, 7, // 71 'G'
    5, 5, 7, 5, 5, // 72 'H'
    2, 2, 2, 2, 2, // 73 'I'
    4, 4, 4, 5, 7, // 74 'J'
    5, 5, 3, 5, 5, // 75 'K'
    1, 1, 1, 1, 7, // 76 'L'
    7, 7, 5, 5, 5, // 77 'M'
    5, 7, 7, 5, 5, // 78 'N'
    7, 5, 5, 5, 7, // 79 'O'
    7, 5, 7, 1, 1, // 80 'P'
    7, 5, 5, 1, 5, // 81 'Q'
    7, 5, 3, 5, 5, // 82 'R'
    7, 1, 7, 4, 7, // 83 'S'
    7, 2, 2, 2, 2, // 84 'T'
    5, 5, 5, 5, 7, // 85 'U'
    5, 5, 5, 5, 2, // 86 'V'
    5, 5, 7, 7, 5, // 87 'W'
    5, 5, 2, 5, 5, // 88 'X'
    5, 5, 7, 2, 2, // 89 'Y'
    7, 4, 2, 1, 7, // 90 'Z'
    0, 0, 6, 2, 2, // 91 '['
    1, 3, 7, 3, 1, // 92 '\\'
    2, 2, 6, 0, 0, // 93 ']'
    2, 2, 2, 2, 2, // 94 '^'
    0, 0, 0, 0, 7, // 95 '_'
    1, 2, 0, 0, 0  // 96 '`'
};
} // namespace

void draw_surface_status_text(std::uint8_t *framebuffer, std::int32_t width, std::int32_t height,
                              const char *text) {
    if (!framebuffer || !text || width <= 0 || height <= 0) return;
    const std::size_t len = std::strlen(text);
    if (len == 0) return;

    const int scale = std::max(1, width / 320);
    const int char_step = 6 * scale;
    const int total_width = static_cast<int>(len) * char_step - (2 * scale);
    const int start_x = (width - total_width) / 2;
    const int start_y = height / 2;

    // Pass 1: Draw black shadow (index 64) offset by (+scale, +scale)
    for (std::size_t n = 0; n < len; ++n) {
        char ch = text[n];
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
        if (ch < 32 || ch > 96) continue;
        const int glyph_idx = (ch - 32) * 5;
        const int gx = start_x + static_cast<int>(n) * char_step;

        for (int row = 0; row < 5; ++row) {
            for (int sy = 0; sy < scale; ++sy) {
                const int py = start_y + (row * scale) + sy + scale;
                if (py < 0 || py >= height) continue;
                const auto bits = surface_hud_font[glyph_idx + row];
                for (int col = 0; col < 3; ++col) {
                    if (bits & (1 << col)) {
                        for (int sx = 0; sx < scale; ++sx) {
                            const int px = gx + (col * scale) + sx + scale;
                            if (px >= 0 && px < width) {
                                framebuffer[py * width + px] = 64; // Dark shadow
                            }
                        }
                    }
                }
            }
        }
    }

    // Pass 2: Draw crisp star-white glyphs (index 127) at (0, 0)
    for (std::size_t n = 0; n < len; ++n) {
        char ch = text[n];
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
        if (ch < 32 || ch > 96) continue;
        const int glyph_idx = (ch - 32) * 5;
        const int gx = start_x + static_cast<int>(n) * char_step;

        for (int row = 0; row < 5; ++row) {
            for (int sy = 0; sy < scale; ++sy) {
                const int py = start_y + (row * scale) + sy;
                if (py < 0 || py >= height) continue;
                const auto bits = surface_hud_font[glyph_idx + row];
                for (int col = 0; col < 3; ++col) {
                    if (bits & (1 << col)) {
                        for (int sx = 0; sx < scale; ++sx) {
                            const int px = gx + (col * scale) + sx;
                            if (px >= 0 && px < width) {
                                framebuffer[py * width + px] = 127; // Crisp star-white
                            }
                        }
                    }
                }
            }
        }
    }
}

const std::vector<std::string_view> &plus_help_lines(bool surface) {
    static const std::vector<std::string_view> space{
        "NOCTIS IV OM - SPACE SHORTCUTS",
        "M/* SNAPSHOT   B/DELETE RAW SNAPSHOT",
        "T/SHIFT+S ROOFSPEED   DOWN MOUSELOOK",
        "F2 EFFECTS   F3 MOVIE   F4 IMAGE ARCHIVE"};
    static const std::vector<std::string_view> ground{
        "NOCTIS IV OM - SURFACE SHORTCUTS",
        "J JUMP   SPACE JETPACK   C RELEASE",
        "L TORCH   M/* SNAP   N// WIDE   B/DEL RAW",
        "DOWN MOUSELOOK   F2 EFFECTS   F3 MOVIE"};
    return surface ? ground : space;
}

const std::vector<std::string> plus_visual_menu_lines(bool draw_hud,
                                                       std::int8_t lens_flare_mode,
                                                       bool seamless_border,
                                                       int timewarp_multiplier,
                                                       int upscale_mode,
                                                       bool crt_shader,
                                                       bool subpixel_fidelity,
                                                       bool show_advanced_fx,
                                                       int internal_res_mode,
                                                       int draw_distance_mode) {
    std::vector<std::string> lines = {
        "NOCTIS IV OM VISUAL EFFECTS SETTINGS",
        draw_hud ? "HUD TEXT ON (T)" : "HUD TEXT OFF (T)",
        lens_flare_mode == 1 ? "LENS FLARES ALWAYS ON (F)"
            : lens_flare_mode == -1 ? "LENS FLARES ALWAYS OFF (F)"
                                    : "VISOR LENS FLARES ONLY (F)",
        seamless_border ? "SEAMLESS BORDER (B)" : "DEFAULT BORDER (B)"};
    if (show_advanced_fx) {
        const char *upscale_label = (upscale_mode == 1) ? "SCALE2X EDGE (U)"
                                  : (upscale_mode == 2) ? "SMOOTH BILINEAR (U)"
                                                        : "CRISP PIXEL 1X (U)";
        lines.emplace_back(std::string("UPSCALE: ") + upscale_label);
        const char *res_label = (internal_res_mode == 1) ? "640X400 2X (R)"
                              : (internal_res_mode == 2) ? "1280X800 4X (R)"
                                                         : "320X200 1X (R)";
        lines.emplace_back(std::string("INTERNAL RES: ") + res_label);
        const char *dist_label = (draw_distance_mode == 1) ? "EXTENDED 96Q (D)"
                               : (draw_distance_mode == 2) ? "FAR 128Q (D)"
                                                           : "STANDARD 64Q (D)";
        lines.emplace_back(std::string("DRAW DISTANCE: ") + dist_label);
        lines.emplace_back(crt_shader ? "CRT SHADER ON (C)" : "CRT SHADER OFF (C)");
        lines.emplace_back(subpixel_fidelity ? "FIDELITY: SUB-PIXEL (G)" : "FIDELITY: LEGACY (G)");
        lines.emplace_back("TAB / A: AUDIO SETTINGS");
    }
    if (timewarp_multiplier > 0) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "TIMEWARP %dx ([ / ] ADJUST)", timewarp_multiplier);
        lines.emplace_back(buf);
    }
    return lines;
}

namespace {
std::string format_audio_slider_row(int index, const char *name, float vol, bool selected) {
    int pct = static_cast<int>(std::round(vol * 100.0f));
    int ticks = std::clamp(static_cast<int>(std::round(vol * 10.0f)), 0, 10);
    char bar[13];
    bar[0] = '[';
    for (int i = 0; i < 10; ++i) {
        bar[1 + i] = (i < ticks) ? '=' : ' ';
    }
    bar[11] = ']';
    bar[12] = '\0';

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%c %d. %-10s %s %3d%%%c",
                  selected ? '>' : ' ',
                  index, name, bar, pct,
                  selected ? '<' : ' ');
    return std::string(buf);
}
} // namespace

const std::vector<std::string> plus_audio_menu_lines(int selected_category,
                                                     float master_vol,
                                                     float cabin_vol,
                                                     float propulsion_vol,
                                                     float weather_vol,
                                                     float foley_vol,
                                                     bool muted) {
    std::vector<std::string> lines = {
        "NOCTIS IV OM AUDIO VOLUME SETTINGS",
        format_audio_slider_row(1, "MASTER:", master_vol, selected_category == 0),
        format_audio_slider_row(2, "CABIN:", cabin_vol, selected_category == 1),
        format_audio_slider_row(3, "PROPULSION:", propulsion_vol, selected_category == 2),
        format_audio_slider_row(4, "WEATHER:", weather_vol, selected_category == 3),
        format_audio_slider_row(5, "FOLEY:", foley_vol, selected_category == 4),
        muted ? "MUTE: AUDIO MUTED (M / F9)" : "MUTE: AUDIO ACTIVE (M / F9)",
        "1-5: SELECT  -/+: ADJUST  TAB: CONTROLS"
    };
    return lines;
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

const std::vector<std::string> plus_controls_menu_lines(bool invert_y,
                                                        float sensitivity,
                                                        int mouselook_mode,
                                                        std::string_view forward_key,
                                                        std::string_view backward_key,
                                                        std::string_view left_key,
                                                        std::string_view right_key,
                                                        bool gamepad_connected,
                                                        std::string_view gamepad_name,
                                                        bool rumble_enabled) {
    char sens_buf[48];
    int ticks = std::clamp(static_cast<int>(std::round(sensitivity * 5.0f)), 0, 15);
    char bar[18];
    bar[0] = '[';
    for (int i = 0; i < 15; ++i) bar[1 + i] = (i < ticks) ? '=' : ' ';
    bar[16] = ']';
    bar[17] = '\0';
    std::snprintf(sens_buf, sizeof(sens_buf), "MOUSE SENSITIVITY: %s %3.1fX (-/+)", bar, sensitivity);

    const char *look_mode_str = (mouselook_mode == 0) ? "OFF (HOLD RMB) (M)"
                              : (mouselook_mode == 1) ? "ON (ALWAYS) (M)"
                                                      : "INV. Y AXIS (M)";

    char move_buf[48];
    std::snprintf(move_buf, sizeof(move_buf), "MOVE: %.*s %.*s %.*s %.*s   JUMP: SPACE",
                  static_cast<int>(forward_key.size()), forward_key.data(),
                  static_cast<int>(left_key.size()), left_key.data(),
                  static_cast<int>(backward_key.size()), backward_key.data(),
                  static_cast<int>(right_key.size()), right_key.data());

    std::string gamepad_line;
    if (gamepad_connected) {
        char gp_buf[64];
        std::snprintf(gp_buf, sizeof(gp_buf), "GAMEPAD: %.*s (CONNECTED)",
                      static_cast<int>(std::min<std::size_t>(gamepad_name.size(), 28)), gamepad_name.data());
        gamepad_line = gp_buf;
    } else {
        gamepad_line = "GAMEPAD: NO CONTROLLER DETECTED";
    }

    const char *rumble_str = rumble_enabled ? "RUMBLE HAPTICS: ACTIVE (R)" : "RUMBLE HAPTICS: MUTED (R)";

    return {
        "NOCTIS IV OM CONTROLS & INPUT SETTINGS",
        invert_y ? "MOUSE PITCH: INVERTED (LOOK DOWN) (I)" : "MOUSE PITCH: NORMAL (LOOK UP) (I)",
        std::string(sens_buf),
        std::string("MOUSELOOK: ") + look_mode_str,
        std::string(move_buf),
        gamepad_line,
        std::string(rumble_str),
        "I: INVERT  -/+: SENS  M: MOUSELOOK  R: RUMBLE  TAB: VIDEO"
    };
}

} // namespace noctis
