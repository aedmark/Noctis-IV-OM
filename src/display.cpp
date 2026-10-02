#include "display.h"
#include "simulation_clock.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace noctis {

namespace {
AspectRatioMode g_current_aspect_mode = AspectRatioMode::crt_4_3;
} // namespace

DisplayViewport calculate_viewport(int window_width, int window_height, AspectRatioMode mode) {
    if (window_width <= 0 || window_height <= 0) {
        return {0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    }

    const float win_w = static_cast<float>(window_width);
    const float win_h = static_cast<float>(window_height);

    if (mode == AspectRatioMode::stretch_16_9) {
        return {0.0f, 0.0f, win_w, win_h, win_h / 200.0f};
    }

    // Determine target aspect ratio (width / height):
    // Mode crt_4_3: 4:3 (~1.3333333) authentic retro CRT proportions
    // Mode pixel_16_10: 16:10 (1.6) square 320x200 pixel ratio
    const float target_aspect = (mode == AspectRatioMode::crt_4_3) ? (4.0f / 3.0f) : 1.6f;
    const float win_aspect    = win_w / win_h;

    float vp_w = 0.0f;
    float vp_h = 0.0f;
    float vp_x = 0.0f;
    float vp_y = 0.0f;

    if (win_aspect > target_aspect) {
        // Window is wider than target -> pillarbox (black bars left and right)
        vp_h = win_h;
        vp_w = std::floor(vp_h * target_aspect);
        vp_x = std::floor((win_w - vp_w) * 0.5f);
        vp_y = 0.0f;
    } else {
        // Window is taller than target -> letterbox (black bars top and bottom)
        vp_w = win_w;
        vp_h = std::floor(vp_w / target_aspect);
        vp_x = 0.0f;
        vp_y = std::floor((win_h - vp_h) * 0.5f);
    }

    const float scale = vp_h / 200.0f;
    return {vp_x, vp_y, vp_w, vp_h, scale};
}

AspectRatioMode cycle_aspect_ratio_mode(AspectRatioMode current) {
    switch (current) {
        case AspectRatioMode::crt_4_3:     return AspectRatioMode::pixel_16_10;
        case AspectRatioMode::pixel_16_10: return AspectRatioMode::stretch_16_9;
        case AspectRatioMode::stretch_16_9: return AspectRatioMode::crt_4_3;
    }
    return AspectRatioMode::crt_4_3;
}

const char *aspect_ratio_mode_name(AspectRatioMode mode) {
    switch (mode) {
        case AspectRatioMode::crt_4_3:     return "ASPECT: 4:3 CRT";
        case AspectRatioMode::pixel_16_10: return "ASPECT: 16:10 SQUARE";
        case AspectRatioMode::stretch_16_9: return "ASPECT: 16:9 STRETCH";
    }
    return "ASPECT: 4:3 CRT";
}

AspectRatioMode get_aspect_ratio_mode() {
    return g_current_aspect_mode;
}

void set_aspect_ratio_mode(AspectRatioMode mode) {
    g_current_aspect_mode = mode;
}

bool is_fullscreen() {
    return IsWindowFullscreen();
}

void toggle_fullscreen() {
    ToggleFullscreen();
}

void render_high_dpi_hud(const char *status_text, int delay, int render_width, int render_height,
                         const DisplayViewport &viewport) {
    if (!status_text || delay <= 0) return;
    const std::size_t len = std::strlen(status_text);
    if (len == 0) return;

    // Fade out smoothly over the final 15 frames
    const float fade = (delay < 15) ? (static_cast<float>(delay) / 15.0f) : 1.0f;
    const auto alpha_fg = static_cast<std::uint8_t>(std::clamp(255.0f * fade, 0.0f, 255.0f));
    const auto alpha_bg = static_cast<std::uint8_t>(std::clamp(205.0f * fade, 0.0f, 205.0f));
    const auto alpha_br = static_cast<std::uint8_t>(std::clamp(170.0f * fade, 0.0f, 170.0f));

    // Dynamic typography scaling based on render viewport height
    const int font_size = std::clamp(static_cast<int>(viewport.height * 0.030f), 14, 28);
    const int text_w    = MeasureText(status_text, font_size);
    const int pad_x     = font_size;
    const int pad_y     = font_size / 2;
    const int pip_size  = static_cast<int>(font_size * 0.6f);
    const int badge_w   = text_w + pad_x * 2 + pip_size;
    const int badge_h   = font_size + pad_y * 2;

    // Position centrally at the top of the viewport
    const float badge_x = viewport.x + (viewport.width - static_cast<float>(badge_w)) * 0.5f;
    const float badge_y = viewport.y + std::max(12.0f, viewport.height * 0.022f);

    const Rectangle pill{badge_x, badge_y, static_cast<float>(badge_w), static_cast<float>(badge_h)};
    const Color bg_color{6, 10, 16, alpha_bg};
    const Color border_color{0, 185, 220, alpha_br};

    // Draw frosted glass backdrop badge with rounded corners
    DrawRectangleRounded(pill, 0.40f, 6, bg_color);
    DrawRectangleRoundedLines(pill, 0.40f, 6, border_color);

    // Glowing cyan/green telemetry status pip
    const float pip_cx = badge_x + static_cast<float>(pad_x) * 0.75f;
    const float pip_cy = badge_y + static_cast<float>(badge_h) * 0.5f;
    const float pip_r  = static_cast<float>(font_size) * 0.20f;
    DrawCircle(static_cast<int>(pip_cx), static_cast<int>(pip_cy), pip_r, Color{0, 230, 195, alpha_fg});

    // Sharp drop shadow and star-white typography
    const int text_x = static_cast<int>(badge_x + pad_x + pip_size);
    const int text_y = static_cast<int>(badge_y + pad_y);
    DrawText(status_text, text_x + 1, text_y + 1, font_size, Color{0, 0, 0, static_cast<std::uint8_t>(180.0f * fade)});
    DrawText(status_text, text_x, text_y, font_size, Color{235, 245, 255, alpha_fg});
}

namespace {
int g_timewarp_slider_cooldown = 0;
}

void touch_timewarp_slider() {
    g_timewarp_slider_cooldown = 120; // Keep visible for ~2 seconds after interaction
}

void render_timewarp_slider(int render_width, int render_height, const DisplayViewport &viewport,
                            int status_delay) {
    if (g_timewarp_slider_cooldown > 0) {
        --g_timewarp_slider_cooldown;
    }

    const bool active = is_timewarp_active();
    if (!active && g_timewarp_slider_cooldown <= 0) {
        return;
    }

    const int font_size = std::clamp(static_cast<int>(viewport.height * 0.025f), 12, 22);
    const float scale   = static_cast<float>(font_size) / 16.0f;
    const float widget_w = std::clamp(390.0f * scale, 280.0f, viewport.width * 0.88f);
    const float widget_h = 36.0f * scale;

    const float widget_x = viewport.x + (viewport.width - widget_w) * 0.5f;
    float widget_y       = viewport.y + std::max(12.0f, viewport.height * 0.022f);
    if (status_delay > 0) {
        widget_y += static_cast<float>(font_size + font_size / 2 * 2 + 10);
    }

    const Rectangle pill{widget_x, widget_y, widget_w, widget_h};
    const Color bg_color{6, 12, 20, 215};
    const Color border_color{0, 185, 220, active ? static_cast<uint8_t>(200) : static_cast<uint8_t>(110)};

    DrawRectangleRounded(pill, 0.40f, 6, bg_color);
    DrawRectangleRoundedLines(pill, 0.40f, 6, border_color);

    // Glowing status dot
    const float dot_cx = widget_x + 14.0f * scale;
    const float dot_cy = widget_y + widget_h * 0.5f;
    const float dot_r  = 4.5f * scale;
    DrawCircle(static_cast<int>(dot_cx), static_cast<int>(dot_cy), dot_r,
               active ? Color{0, 240, 200, 255} : Color{120, 140, 160, 180});

    // Label: "TIME"
    const char *label = "TIME:";
    const int label_x = static_cast<int>(dot_cx + 10.0f * scale);
    const int label_y = static_cast<int>(widget_y + (widget_h - font_size) * 0.5f);
    DrawText(label, label_x, label_y, font_size, Color{200, 230, 250, 230});

    const int label_w = MeasureText(label, font_size);

    // Step button [-]
    const float btn_w = 20.0f * scale;
    const float btn_h = 20.0f * scale;
    const float btn_dec_x = label_x + label_w + 10.0f * scale;
    const float btn_y = widget_y + (widget_h - btn_h) * 0.5f;
    const Rectangle dec_rec{btn_dec_x, btn_y, btn_w, btn_h};

    DrawRectangleRounded(dec_rec, 0.3f, 4, Color{20, 35, 50, 180});
    DrawRectangleRoundedLines(dec_rec, 0.3f, 4, Color{0, 180, 210, 130});
    DrawText("-", static_cast<int>(btn_dec_x + 6.0f * scale), static_cast<int>(btn_y + 2.0f * scale), font_size, Color{220, 240, 255, 230});

    // Track
    const float track_x = btn_dec_x + btn_w + 8.0f * scale;
    const float text_val_w = 60.0f * scale;
    const float track_w = std::max(60.0f, widget_x + widget_w - track_x - btn_w - text_val_w - 16.0f * scale);
    const float track_h = 6.0f * scale;
    const float track_y = widget_y + (widget_h - track_h) * 0.5f;

    // Step button [+]
    const float btn_inc_x = track_x + track_w + 8.0f * scale;
    const Rectangle inc_rec{btn_inc_x, btn_y, btn_w, btn_h};
    DrawRectangleRounded(inc_rec, 0.3f, 4, Color{20, 35, 50, 180});
    DrawRectangleRoundedLines(inc_rec, 0.3f, 4, Color{0, 180, 210, 130});
    DrawText("+", static_cast<int>(btn_inc_x + 5.0f * scale), static_cast<int>(btn_y + 2.0f * scale), font_size, Color{220, 240, 255, 230});

    // Multiplier text
    const int current_mult = get_timewarp_multiplier();
    char mult_str[24];
    std::snprintf(mult_str, sizeof(mult_str), "%dx", current_mult);
    const int mult_text_x = static_cast<int>(btn_inc_x + btn_w + 8.0f * scale);
    DrawText(mult_str, mult_text_x, label_y, font_size,
             active ? Color{0, 240, 210, 255} : Color{180, 200, 215, 200});

    // Draw track
    DrawRectangleRounded(Rectangle{track_x, track_y, track_w, track_h}, 0.5f, 4, Color{15, 30, 45, 200});
    const float frac = timewarp_fraction_from_multiplier(current_mult);
    const float fill_w = track_w * frac;
    if (fill_w > 1.0f) {
        DrawRectangleRounded(Rectangle{track_x, track_y, fill_w, track_h}, 0.5f, 4, Color{0, 200, 210, 220});
    }

    // Knob
    const float knob_cx = track_x + fill_w;
    const float knob_cy = track_y + track_h * 0.5f;
    const float knob_r  = 6.0f * scale;
    DrawCircle(static_cast<int>(knob_cx), static_cast<int>(knob_cy), knob_r, Color{0, 230, 240, 255});
    DrawCircle(static_cast<int>(knob_cx), static_cast<int>(knob_cy), knob_r * 0.45f, Color{255, 255, 255, 255});

    // Mouse Interaction
    const Vector2 mouse = GetMousePosition();
    const bool mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    const Rectangle track_hitbox{track_x - 8.0f, widget_y, track_w + 16.0f, widget_h};

    if (mouse_pressed && CheckCollisionPointRec(mouse, dec_rec)) {
        step_timewarp_multiplier(-1);
        touch_timewarp_slider();
    } else if (mouse_pressed && CheckCollisionPointRec(mouse, inc_rec)) {
        step_timewarp_multiplier(+1);
        touch_timewarp_slider();
    } else if (mouse_down && CheckCollisionPointRec(mouse, track_hitbox)) {
        const float new_frac = std::clamp((mouse.x - track_x) / track_w, 0.0f, 1.0f);
        const int new_mult = timewarp_multiplier_from_fraction(new_frac);
        set_timewarp_multiplier(new_mult);
        touch_timewarp_slider();
    }
}

} // namespace noctis
