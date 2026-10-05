#pragma once

#include "atmospheric_scattering.h"
#include "stellar_coronal_flares.h"
#include "upscale.h"

#include <cstdint>
#include <filesystem>
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
void set_fullscreen(bool enabled);

// Presentation setting accessors for display config
std::int8_t get_setting_draw_hud();
void set_setting_draw_hud(std::int8_t val);
std::int8_t get_setting_lens_flare_mode();
void set_setting_lens_flare_mode(std::int8_t val);
std::int8_t get_setting_seamless_border();
void set_setting_seamless_border(std::int8_t val);

// High-DPI HUD overlay rendering
void render_high_dpi_hud(const char *status_text, int delay, int render_width, int render_height,
                         const DisplayViewport &viewport);

// Transient high-DPI notice that never enters the indexed framebuffer, so it
// cannot appear in snapshots or Moviemaker frames.
void show_overlay_notice(const char *text, int frames = 60);
// Draws the active notice; returns false when none is showing.
bool render_overlay_notice(int render_width, int render_height, const DisplayViewport &viewport);

// High-DPI Timewarp slider widget rendering and mouse interaction
void render_timewarp_slider(int render_width, int render_height, const DisplayViewport &viewport,
                            int status_delay = 0);
void touch_timewarp_slider();
bool is_timewarp_slider_visible();

// High-DPI Volume slider widget rendering and mouse interaction
void render_volume_slider_overlay(int render_width, int render_height, const DisplayViewport &viewport,
                                  int status_delay = 0, bool pinned = false);
void touch_volume_slider();
bool is_volume_slider_visible();

// CRT Shader Pipeline (M10-W02)
bool is_crt_shader_enabled();
void set_crt_shader_enabled(bool enabled);
bool toggle_crt_shader();
void init_display_shaders();
void cleanup_display_shaders();
void begin_crt_shader(int render_width, int render_height);
void end_crt_shader();

// Sub-pixel geometry fidelity mode (M10-W03)
#ifndef NOCTIS_SUBPIXEL_FIDELITY_DEFINED
#define NOCTIS_SUBPIXEL_FIDELITY_DEFINED
inline bool g_subpixel_fidelity = false;
inline bool get_subpixel_fidelity() { return g_subpixel_fidelity; }
inline void set_subpixel_fidelity(bool enabled) { g_subpixel_fidelity = enabled; }
inline bool toggle_subpixel_fidelity() { g_subpixel_fidelity = !g_subpixel_fidelity; return g_subpixel_fidelity; }
#endif

// Internal resolution modes (Milestone 13 - Phase 1)
enum class InternalResolutionMode : std::uint8_t {
    res_1x, // 320x200 (1x Authentic)
    res_2x, // 640x400 (2x Enhanced)
    res_4x  // 1280x800 (4x Ultra)
};

InternalResolutionMode get_internal_resolution_mode();
void set_internal_resolution_mode(InternalResolutionMode mode);
InternalResolutionMode cycle_internal_resolution_mode(InternalResolutionMode current);
InternalResolutionMode cycle_internal_resolution_mode();
const char *internal_resolution_mode_name(InternalResolutionMode mode);
int internal_resolution_scale(InternalResolutionMode mode);
using InternalResolutionChangeCallback = void (*)(InternalResolutionMode mode);
void set_internal_resolution_change_callback(InternalResolutionChangeCallback cb);

// Terrain draw distance modes (Milestone 13 - Phase 2)
enum class DrawDistanceMode : std::uint8_t {
    standard, // 64 quadrants (1.0x Authentic DOS)
    extended, // 96 quadrants (1.5x Extended horizon)
    far       // 128 quadrants (2.0x Distant horizon, full active heightfield)
};

DrawDistanceMode get_draw_distance_mode();
void set_draw_distance_mode(DrawDistanceMode mode);
DrawDistanceMode cycle_draw_distance_mode(DrawDistanceMode current);
DrawDistanceMode cycle_draw_distance_mode();
const char *draw_distance_mode_name(DrawDistanceMode mode);
int draw_distance_max_depth(DrawDistanceMode mode);

// Surface texture filtering and micro-detail modes (Milestone 13 - Phase 3)
enum class TextureFilterMode : std::uint8_t {
    nearest,  // 0: Authentic DOS nearest-neighbor point sampling
    bilinear, // 1: Sub-texel bilinear filtering
    detailed  // 2: Bilinear filtering + procedural micro-surface grain
};

inline TextureFilterMode g_texture_filter_mode = TextureFilterMode::nearest;

inline TextureFilterMode get_texture_filter_mode() {
    return g_texture_filter_mode;
}

inline void set_texture_filter_mode(TextureFilterMode mode) {
    g_texture_filter_mode = mode;
}

inline TextureFilterMode cycle_texture_filter_mode(TextureFilterMode current) {
    switch (current) {
        case TextureFilterMode::nearest:  return TextureFilterMode::bilinear;
        case TextureFilterMode::bilinear: return TextureFilterMode::detailed;
        case TextureFilterMode::detailed: return TextureFilterMode::nearest;
    }
    return TextureFilterMode::nearest;
}

inline TextureFilterMode cycle_texture_filter_mode() {
    auto next = cycle_texture_filter_mode(g_texture_filter_mode);
    set_texture_filter_mode(next);
    return next;
}

inline const char *texture_filter_mode_name(TextureFilterMode mode) {
    switch (mode) {
        case TextureFilterMode::nearest:  return "TEXTURE FILTER: NEAREST";
        case TextureFilterMode::bilinear: return "TEXTURE FILTER: BILINEAR";
        case TextureFilterMode::detailed: return "TEXTURE FILTER: DETAILED";
    }
    return "TEXTURE FILTER: NEAREST";
}

// Display Settings Structure & Persistence (Milestone 10)
struct DisplaySettings {
    AspectRatioMode aspect_ratio = AspectRatioMode::crt_4_3;
    UpscaleMode upscale_mode = UpscaleMode::crisp_pixel;
    bool crt_shader = false;
    bool subpixel_fidelity = false;
    InternalResolutionMode internal_resolution = InternalResolutionMode::res_1x;
    DrawDistanceMode draw_distance = DrawDistanceMode::standard;
    TextureFilterMode texture_filter = TextureFilterMode::nearest;
    AtmosphericScatteringMode atmospheric_scattering = AtmosphericScatteringMode::authentic;
    CoronalFlaresMode coronal_flares = CoronalFlaresMode::authentic;
    bool fullscreen = false;
    int timewarp_multiplier = 100;
    std::int8_t draw_hud = 1;
    std::int8_t lens_flare_mode = 0;
    std::int8_t seamless_border = 0;
};

DisplaySettings capture_display_settings();
void apply_display_settings(const DisplaySettings &settings);
bool save_display_settings(const std::filesystem::path &config_dir);
bool load_display_settings(const std::filesystem::path &config_dir);

} // namespace noctis
