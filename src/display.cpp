#include "display.h"
#include "simulation_clock.h"

#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace noctis {

namespace {
AspectRatioMode g_current_aspect_mode = AspectRatioMode::crt_4_3;
bool g_fullscreen_configured = false;
std::int8_t g_draw_hud = 1;
std::int8_t g_lens_flare_mode = 0;
std::int8_t g_seamless_border = 0;
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
#ifdef __EMSCRIPTEN__
    // web/shell.html owns browser fullscreen (it must be requested inside a
    // user-gesture handler) and mirrors the target state here.
    return EM_ASM_INT({ return Module.nivlrFullscreen ? 1 : 0; }) != 0;
#else
    return IsWindowReady() ? IsWindowFullscreen() : g_fullscreen_configured;
#endif
}

void toggle_fullscreen() {
#ifdef __EMSCRIPTEN__
    // The page already toggled fullscreen in its F11 / Alt+Enter handler.
    g_fullscreen_configured = is_fullscreen();
#else
    if (IsWindowReady()) {
        ToggleFullscreen();
        g_fullscreen_configured = IsWindowFullscreen();
    } else {
        g_fullscreen_configured = !g_fullscreen_configured;
    }
#endif
}

void set_fullscreen(bool enabled) {
    g_fullscreen_configured = enabled;
#ifndef __EMSCRIPTEN__
    // Browsers refuse fullscreen without a user gesture, so a saved
    // preference cannot be restored on page load.
    if (IsWindowReady() && (IsWindowFullscreen() != enabled)) {
        ToggleFullscreen();
    }
#endif
}

std::int8_t get_setting_draw_hud() { return g_draw_hud; }
void set_setting_draw_hud(std::int8_t val) { g_draw_hud = val; }
std::int8_t get_setting_lens_flare_mode() { return g_lens_flare_mode; }
void set_setting_lens_flare_mode(std::int8_t val) { g_lens_flare_mode = val; }
std::int8_t get_setting_seamless_border() { return g_seamless_border; }
void set_setting_seamless_border(std::int8_t val) { g_seamless_border = val; }

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
char g_notice_text[48]{};
int g_notice_frames = 0;
}

void show_overlay_notice(const char *text, int frames) {
    std::snprintf(g_notice_text, sizeof(g_notice_text), "%s", text ? text : "");
    g_notice_frames = frames;
}

bool render_overlay_notice(int render_width, int render_height, const DisplayViewport &viewport) {
    if (g_notice_frames <= 0) return false;
    render_high_dpi_hud(g_notice_text, g_notice_frames--, render_width, render_height, viewport);
    return true;
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

namespace {
bool g_crt_shader_enabled = false;
Shader g_crt_shader{};
bool g_shader_loaded      = false;
int g_loc_resolution      = -1;
int g_loc_time            = -1;

#if defined(__EMSCRIPTEN__)
// WebGL2 (GLSL ES 3.00) shares the GLSL 3.30 in/out syntax; only the header differs.
#define NIVLR_GLSL_HEADER "#version 300 es\nprecision highp float;\n"
#else
#define NIVLR_GLSL_HEADER "#version 330\n"
#endif

const char *kCrtFragmentShader = NIVLR_GLSL_HEADER R"(
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;
uniform float time;

// Gentle barrel distortion: authentic curved glass monitor
vec2 curve(vec2 uv) {
    vec2 st = uv * 2.0 - 1.0;
    vec2 offset = abs(st.yx) / vec2(8.0, 6.0);
    st = st + st * offset * offset;
    return st * 0.5 + 0.5;
}

void main() {
    vec2 uv = curve(fragTexCoord);

    // Dark border outside curved monitor glass
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 tex = texture(texture0, uv);

    // 1. Scanline modulation (200 vertical scanlines corresponding to authentic 320x200 CRT)
    float scanline = 0.88 + 0.12 * sin(uv.y * 200.0 * 6.2831853);

    // 2. Aperture grille / shadow mask phosphor triad simulation
    float px = mod(gl_FragCoord.x, 3.0);
    vec3 mask = vec3(0.92);
    if (px < 1.0) {
        mask.r = 1.08;
    } else if (px < 2.0) {
        mask.g = 1.08;
    } else {
        mask.b = 1.08;
    }

    // 3. Phosphor bloom / core glow for intense celestial bodies and stars
    float lum = dot(tex.rgb, vec3(0.299, 0.587, 0.114));
    vec3 bloom = vec3(0.0);
    if (lum > 0.65) {
        bloom = (lum - 0.65) * 0.3 * tex.rgb;
    }

    // 4. Subtle corner vignette
    vec2 vignette_coord = uv * (1.0 - uv);
    float vignette = clamp(vignette_coord.x * vignette_coord.y * 25.0, 0.0, 1.0);
    vignette = pow(vignette, 0.25);

    vec3 rgb = (tex.rgb + bloom) * scanline * mask * vignette;
    finalColor = vec4(rgb, tex.a) * colDiffuse * fragColor;
}
)";
} // namespace

void init_display_shaders() {
    if (g_shader_loaded) return;
    g_crt_shader = LoadShaderFromMemory(nullptr, kCrtFragmentShader);
    if (IsShaderValid(g_crt_shader)) {
        g_shader_loaded  = true;
        g_loc_resolution = GetShaderLocation(g_crt_shader, "resolution");
        g_loc_time       = GetShaderLocation(g_crt_shader, "time");
    }
}

void cleanup_display_shaders() {
    if (g_shader_loaded && IsShaderValid(g_crt_shader)) {
        UnloadShader(g_crt_shader);
    }
    g_shader_loaded  = false;
    g_crt_shader     = {};
    g_loc_resolution = -1;
    g_loc_time       = -1;
}

void begin_crt_shader(int render_width, int render_height) {
    if (!g_crt_shader_enabled) return;
    if (!g_shader_loaded) {
        init_display_shaders();
    }
    if (!g_shader_loaded || !IsShaderValid(g_crt_shader)) return;

    if (g_loc_resolution >= 0) {
        const float res[2] = {static_cast<float>(render_width), static_cast<float>(render_height)};
        SetShaderValue(g_crt_shader, g_loc_resolution, res, SHADER_UNIFORM_VEC2);
    }
    if (g_loc_time >= 0) {
        const float t = static_cast<float>(GetTime());
        SetShaderValue(g_crt_shader, g_loc_time, &t, SHADER_UNIFORM_FLOAT);
    }
    BeginShaderMode(g_crt_shader);
}

void end_crt_shader() {
    if (!g_crt_shader_enabled || !g_shader_loaded || !IsShaderValid(g_crt_shader)) return;
    EndShaderMode();
}

bool is_crt_shader_enabled() {
    return g_crt_shader_enabled;
}

void set_crt_shader_enabled(bool enabled) {
    g_crt_shader_enabled = enabled;
}

bool toggle_crt_shader() {
    g_crt_shader_enabled = !g_crt_shader_enabled;
    return g_crt_shader_enabled;
}

namespace {

inline std::string trim_str(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
    return std::string(s);
}

inline std::string to_lower_str(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return res;
}

} // namespace

DisplaySettings capture_display_settings() {
    DisplaySettings s;
    s.aspect_ratio = get_aspect_ratio_mode();
    s.upscale_mode = get_upscale_mode();
    s.crt_shader = is_crt_shader_enabled();
    s.subpixel_fidelity = get_subpixel_fidelity();
    s.fullscreen = is_fullscreen();
    s.timewarp_multiplier = get_timewarp_multiplier();
    s.draw_hud = g_draw_hud;
    s.lens_flare_mode = g_lens_flare_mode;
    s.seamless_border = g_seamless_border;
    return s;
}

void apply_display_settings(const DisplaySettings &settings) {
    set_aspect_ratio_mode(settings.aspect_ratio);
    set_upscale_mode(settings.upscale_mode);
    set_crt_shader_enabled(settings.crt_shader);
    set_subpixel_fidelity(settings.subpixel_fidelity);
    set_fullscreen(settings.fullscreen);
    set_timewarp_multiplier(settings.timewarp_multiplier);
    g_draw_hud = settings.draw_hud;
    g_lens_flare_mode = settings.lens_flare_mode;
    g_seamless_border = settings.seamless_border;
}

bool save_display_settings(const std::filesystem::path &config_dir) {
    std::error_code ec;
    std::filesystem::create_directories(config_dir, ec);
    if (ec) return false;

    const auto settings = capture_display_settings();
    const auto file_path = config_dir / "display_settings.ini";
    const auto tmp_path = config_dir / "display_settings.ini.tmp";

    std::ofstream out(tmp_path, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "[Display]\n";

    const char *aspect_str = "crt_4_3";
    if (settings.aspect_ratio == AspectRatioMode::pixel_16_10) aspect_str = "pixel_16_10";
    else if (settings.aspect_ratio == AspectRatioMode::stretch_16_9) aspect_str = "stretch_16_9";
    out << "aspect_ratio = " << aspect_str << "\n";

    const char *upscale_str = "crisp";
    if (settings.upscale_mode == UpscaleMode::edge_scale2x) upscale_str = "scale2x";
    else if (settings.upscale_mode == UpscaleMode::smooth_bilinear) upscale_str = "smooth";
    out << "upscale_mode = " << upscale_str << "\n";

    out << "crt_shader = " << (settings.crt_shader ? 1 : 0) << "\n";
    out << "subpixel_fidelity = " << (settings.subpixel_fidelity ? 1 : 0) << "\n";
    out << "fullscreen = " << (settings.fullscreen ? 1 : 0) << "\n";

    out << "timewarp_multiplier = " << settings.timewarp_multiplier << "\n";
    out << "draw_hud = " << static_cast<int>(settings.draw_hud) << "\n";
    out << "lens_flare_mode = " << static_cast<int>(settings.lens_flare_mode) << "\n";
    out << "seamless_border = " << static_cast<int>(settings.seamless_border) << "\n";

    out.close();
    if (!out) return false;

    std::filesystem::rename(tmp_path, file_path, ec);
    if (ec) {
        std::filesystem::copy_file(tmp_path, file_path, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(tmp_path, ec);
    }
    return !ec;
}

bool load_display_settings(const std::filesystem::path &config_dir) {
    const auto file_path = config_dir / "display_settings.ini";
    std::error_code ec;
    if (!std::filesystem::exists(file_path, ec)) return false;

    std::ifstream in(file_path);
    if (!in.is_open()) return false;

    DisplaySettings settings = capture_display_settings();
    std::string line;

    while (std::getline(in, line)) {
        const auto comment_pos = line.find_first_of(";#");
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        std::string trimmed = trim_str(line);
        if (trimmed.empty() || trimmed.front() == '[') continue;

        const auto eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = to_lower_str(trim_str(trimmed.substr(0, eq_pos)));
        std::string val = to_lower_str(trim_str(trimmed.substr(eq_pos + 1)));

        if (key == "aspect_ratio") {
            if (val == "crt_4_3" || val == "0" || val == "4:3") settings.aspect_ratio = AspectRatioMode::crt_4_3;
            else if (val == "pixel_16_10" || val == "1" || val == "16:10") settings.aspect_ratio = AspectRatioMode::pixel_16_10;
            else if (val == "stretch_16_9" || val == "2" || val == "16:9") settings.aspect_ratio = AspectRatioMode::stretch_16_9;
        } else if (key == "upscale_mode") {
            if (val == "crisp" || val == "crisp_pixel" || val == "0") settings.upscale_mode = UpscaleMode::crisp_pixel;
            else if (val == "scale2x" || val == "edge_scale2x" || val == "1") settings.upscale_mode = UpscaleMode::edge_scale2x;
            else if (val == "smooth" || val == "smooth_bilinear" || val == "2") settings.upscale_mode = UpscaleMode::smooth_bilinear;
        } else if (key == "crt_shader") {
            settings.crt_shader = (val == "1" || val == "true" || val == "on" || val == "yes");
        } else if (key == "subpixel_fidelity") {
            settings.subpixel_fidelity = (val == "1" || val == "true" || val == "on" || val == "yes");
        } else if (key == "fullscreen") {
            settings.fullscreen = (val == "1" || val == "true" || val == "on" || val == "yes");
        } else if (key == "timewarp_multiplier") {
            try {
                int mult = std::stoi(val);
                settings.timewarp_multiplier = std::clamp(mult, 1, 5000);
            } catch (...) {}
        } else if (key == "draw_hud") {
            try {
                settings.draw_hud = (std::stoi(val) != 0) ? 1 : 0;
            } catch (...) {}
        } else if (key == "lens_flare_mode") {
            try {
                int mode = std::stoi(val);
                settings.lens_flare_mode = static_cast<std::int8_t>(std::clamp(mode, -1, 1));
            } catch (...) {}
        } else if (key == "seamless_border") {
            try {
                settings.seamless_border = (std::stoi(val) != 0) ? 1 : 0;
            } catch (...) {}
        }
    }

    apply_display_settings(settings);
    return true;
}

} // namespace noctis
