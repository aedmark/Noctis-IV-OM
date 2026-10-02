#pragma once

#include <cstdint>
#include <string>

namespace noctis {

enum class AspectRatioMode : std::uint8_t {
    crt_4_3,     // Authentic 4:3 CRT proportions (canonical round celestial bodies)
    pixel_16_10, // 16:10 square pixels (320x200 pixel-exact 1.6:1)
    stretch_16_9 // Stretch to full window bounds
};

struct DisplayViewport {
    float x      = 0.0f;
    float y      = 0.0f;
    float width  = 0.0f;
    float height = 0.0f;
    float scale  = 1.0f;
};

// Pure function for computing destination viewport inside arbitrary window/render dimensions
DisplayViewport calculate_viewport(int window_width, int window_height, AspectRatioMode mode);

// Cycle to the next aspect ratio mode
AspectRatioMode cycle_aspect_ratio_mode(AspectRatioMode current);

// User-facing display status name
const char *aspect_ratio_mode_name(AspectRatioMode mode);

// Display runtime state
AspectRatioMode get_aspect_ratio_mode();
void set_aspect_ratio_mode(AspectRatioMode mode);

// Fullscreen controls
bool is_fullscreen();
void toggle_fullscreen();

// High-DPI HUD overlay rendering
void render_high_dpi_hud(const char *status_text, int delay, int render_width, int render_height,
                         const DisplayViewport &viewport);

// High-DPI Timewarp slider widget rendering and mouse interaction
void render_timewarp_slider(int render_width, int render_height, const DisplayViewport &viewport,
                            int status_delay = 0);
void touch_timewarp_slider();

} // namespace noctis
