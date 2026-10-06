#include "movie_player.h"

#include "audio.h"
#include "engine_state.h"
#include "gallery.h"
#include "input.h"
#include "runtime_paths.h"
#include "video_export.h"

#include <raylib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace noctis {

namespace {

constexpr Color backdrop_color{4, 8, 14, 230};
constexpr Color panel_color{8, 14, 22, 240};
constexpr Color border_color{0, 185, 220, 200};
constexpr Color title_color{0, 230, 195, 255};
constexpr Color text_color{235, 245, 255, 255};
constexpr Color hint_color{150, 170, 190, 255};
constexpr Color played_bar_color{0, 210, 240, 230};
constexpr Color track_bar_color{25, 40, 60, 200};

constexpr std::array<double, 6> available_framerates = {6.0, 12.0, 18.2, 24.0, 30.0, 60.0};

struct MoviePlayerState {
    bool open = false;
    MovieDeckEntry deck;
    std::vector<std::filesystem::path> frame_files;
    std::size_t current_frame = 0;
    bool playing = true;
    bool looping = true;
    double fps = 18.2;
    float time_acc = 0.0f;
    bool scrubbing = false;

    Texture2D texture{};
    std::size_t loaded_frame = std::numeric_limits<std::size_t>::max();

    std::string toast_text;
    float toast_timer = 0.0f;
    bool toast_success = true;
};

MoviePlayerState g_player;

void release_player_texture() {
    if (g_player.texture.id != 0) {
        UnloadTexture(g_player.texture);
        g_player.texture = {};
    }
    g_player.loaded_frame = std::numeric_limits<std::size_t>::max();
}

void ensure_player_frame_texture() {
    if (g_player.frame_files.empty() || g_player.current_frame >= g_player.frame_files.size()) {
        return;
    }
    if (g_player.loaded_frame == g_player.current_frame && g_player.texture.id != 0) {
        return;
    }

    auto decoded = load_gallery_image(g_player.frame_files[g_player.current_frame]);
    if (!decoded) return;

    if (g_player.texture.id == 0 || g_player.texture.width != decoded->width || g_player.texture.height != decoded->height) {
        release_player_texture();
        const Image img{decoded->rgba.data(), decoded->width, decoded->height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        g_player.texture = LoadTextureFromImage(img);
        SetTextureFilter(g_player.texture, TEXTURE_FILTER_POINT);
    } else {
        UpdateTexture(g_player.texture, decoded->rgba.data());
    }
    g_player.loaded_frame = g_player.current_frame;
}

void show_toast(const std::string &message, bool success = true, float duration = 3.5f) {
    g_player.toast_text = message;
    g_player.toast_success = success;
    g_player.toast_timer = duration;
}

void draw_text_shadow(const char *text, int x, int y, int size, Color color) {
    DrawText(text, x + 1, y + 1, size, Color{0, 0, 0, 180});
    DrawText(text, x, y, size, color);
}

bool draw_ui_button(const Rectangle &rect, const char *label, int font_size) {
    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, rect);
    const bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    const Color bg = hovered ? Color{0, 185, 220, 60} : Color{12, 22, 34, 215};
    const Color border = hovered ? title_color : border_color;
    const Color text_col = hovered ? title_color : text_color;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 1.0F, border);
    const int text_w = MeasureText(label, font_size);
    const int text_x = static_cast<int>(rect.x + (rect.width - static_cast<float>(text_w)) * 0.5F);
    const int text_y = static_cast<int>(rect.y + (rect.height - static_cast<float>(font_size)) * 0.5F);
    draw_text_shadow(label, text_x, text_y, font_size, text_col);
    if (clicked) {
        play_cockpit_button();
    }
    return clicked;
}

void trigger_movie_export() {
    if (g_player.frame_files.empty()) return;
#if defined(__EMSCRIPTEN__)
    show_toast("IN BROWSER: USE F3 ENTER FOR WEBM RECORDING", true);
    play_goesnet_chime(true);
#else
    VideoExportOptions options;
    options.deck_dir = g_player.deck.path;
    options.fps = g_player.fps;
    options.format = VideoFormat::mp4;

    if (start_video_export_async(options)) {
        show_toast("EXPORTING DECK " + g_player.deck.deck_str + " TO MP4...", true, 5.0f);
        play_goesnet_chime(true);
    } else {
        show_toast("EXPORT ALREADY IN PROGRESS", false);
        play_goesnet_chime(false);
    }
#endif
}

void trigger_movie_open_folder() {
#ifndef __EMSCRIPTEN__
    if (!g_player.deck.path.empty()) {
        const bool ok = open_gallery_folder(g_player.deck.path);
        play_goesnet_chime(ok);
        show_toast(ok ? "OPENED MOVIE DECK FOLDER" : "COULD NOT OPEN FOLDER", ok);
    }
#endif
}

void step_framerate(int direction) {
    auto it = std::lower_bound(available_framerates.begin(), available_framerates.end(), g_player.fps - 0.1);
    int idx = static_cast<int>(std::distance(available_framerates.begin(), it));
    if (idx >= static_cast<int>(available_framerates.size())) idx = static_cast<int>(available_framerates.size()) - 1;

    idx += direction;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(available_framerates.size())) idx = static_cast<int>(available_framerates.size()) - 1;

    g_player.fps = available_framerates[idx];
    char msg[32];
    std::snprintf(msg, sizeof(msg), "SPEED: %.1f FPS", g_player.fps);
    show_toast(msg, true, 1.5f);
}

} // namespace

bool open_movie_player(const std::filesystem::path &movies_dir, std::uint16_t deck) {
    auto decks = scan_movie_decks(movies_dir);
    if (decks.empty()) return false;

    const MovieDeckEntry *chosen = nullptr;
    if (deck == 0) {
        chosen = &decks.back(); // Latest recorded deck
    } else {
        for (const auto &d : decks) {
            if (d.deck == deck) {
                chosen = &d;
                break;
            }
        }
    }
    if (!chosen) return false;

    std::vector<std::filesystem::path> frames;
    std::error_code ec;
    for (const auto &sub : std::filesystem::directory_iterator(chosen->path, ec)) {
        if (ec) break;
        if (!sub.is_regular_file(ec) || ec) continue;
        const auto ext = sub.path().extension().string();
        if (ext == ".BMP" || ext == ".bmp") {
            frames.push_back(sub.path());
        }
    }
    if (frames.empty()) return false;

    std::sort(frames.begin(), frames.end());

    if (!engine_state().application.transition_to(ApplicationMode::movie_player)) return false;

    release_player_texture();
    g_player = MoviePlayerState{};
    g_player.open = true;
    g_player.deck = *chosen;
    g_player.frame_files = std::move(frames);
    g_player.current_frame = 0;
    g_player.playing = true;
    g_player.looping = true;
    g_player.fps = chosen->fps > 0.0 ? chosen->fps : 18.2;
    g_player.time_acc = 0.0f;

#ifndef __EMSCRIPTEN__
    if (IsWindowReady()) EnableCursor();
#else
    EM_ASM({ if (document.exitPointerLock) document.exitPointerLock(); });
#endif
    play_goesnet_chime(true);
    return true;
}

bool movie_player_open() {
    return g_player.open;
}

void close_movie_player() {
    if (!g_player.open) return;
    g_player.open = false;
    if (engine_state().application.mode == ApplicationMode::movie_player) {
        (void) engine_state().application.transition_to(ApplicationMode::cockpit);
    }
    release_player_texture();
    g_player.frame_files.clear();
#ifndef __EMSCRIPTEN__
    if (IsWindowReady() && is_cursor_lock_wanted()) DisableCursor();
#endif
}

void shutdown_movie_player() {
    close_movie_player();
}

std::uint16_t movie_player_current_deck() {
    return g_player.deck.deck;
}

std::size_t movie_player_current_frame() {
    return g_player.current_frame;
}

std::size_t movie_player_total_frames() {
    return g_player.frame_files.size();
}

bool movie_player_is_playing() {
    return g_player.playing;
}

bool movie_player_is_looping() {
    return g_player.looping;
}

double movie_player_fps() {
    return g_player.fps;
}

bool movie_player_input(const InputFrame &frame) {
    if (!g_player.open) return false;

    if (frame.escape_down || frame.cancel_pressed) {
        close_movie_player();
        play_cockpit_button();
        return true;
    }

    if (frame.space_pressed) {
        g_player.playing = !g_player.playing;
        show_toast(g_player.playing ? "PLAYING" : "PAUSED", true, 1.2f);
        play_cockpit_button();
        return true;
    }

    if (frame.arrow_left_pressed) {
        g_player.playing = false;
        if (g_player.current_frame > 0) {
            --g_player.current_frame;
        } else if (g_player.looping && !g_player.frame_files.empty()) {
            g_player.current_frame = g_player.frame_files.size() - 1;
        }
        play_cockpit_button();
        return true;
    }

    if (frame.arrow_right_pressed) {
        g_player.playing = false;
        if (g_player.current_frame + 1 < g_player.frame_files.size()) {
            ++g_player.current_frame;
        } else if (g_player.looping) {
            g_player.current_frame = 0;
        }
        play_cockpit_button();
        return true;
    }

    if (frame.home_pressed) {
        g_player.current_frame = 0;
        g_player.playing = false;
        play_cockpit_button();
        return true;
    }

    if (frame.end_pressed && !g_player.frame_files.empty()) {
        g_player.current_frame = g_player.frame_files.size() - 1;
        g_player.playing = false;
        play_cockpit_button();
        return true;
    }

    if (frame.arrow_up_pressed || frame.plus_pressed) {
        step_framerate(1);
        play_cockpit_button();
        return true;
    }

    if (frame.arrow_down_pressed || frame.minus_pressed) {
        step_framerate(-1);
        play_cockpit_button();
        return true;
    }

    for (int key : frame.text) {
        if (key == 'p' || key == 'P') {
            g_player.playing = !g_player.playing;
            show_toast(g_player.playing ? "PLAYING" : "PAUSED", true, 1.2f);
            play_cockpit_button();
            return true;
        }
        if (key == 'l' || key == 'L') {
            g_player.looping = !g_player.looping;
            show_toast(g_player.looping ? "LOOP ENABLED" : "LOOP DISABLED", true, 1.5f);
            play_cockpit_button();
            return true;
        }
        if (key == 'x' || key == 'X' || key == 'e' || key == 'E') {
            trigger_movie_export();
            return true;
        }
        if (key == 'o' || key == 'O') {
            trigger_movie_open_folder();
            return true;
        }
    }

    return true;
}

void render_movie_player(int render_width, int render_height, const DisplayViewport &viewport) {
    (void)viewport;
    if (!g_player.open || g_player.frame_files.empty()) return;

    // Background video export status polling
    if (!is_video_export_running()) {
        const auto exp_status = get_video_export_status();
        if (exp_status.finished && !exp_status.message.empty()) {
            show_toast(exp_status.message, exp_status.success, 4.0f);
            play_goesnet_chime(exp_status.success);
            reset_video_export_status();
        }
    }

    // Playback timeline update
    const float dt = GetFrameTime();
    if (g_player.toast_timer > 0.0f) {
        g_player.toast_timer -= dt;
        if (g_player.toast_timer < 0.0f) g_player.toast_timer = 0.0f;
    }

    if (g_player.playing && !g_player.scrubbing && g_player.frame_files.size() > 1) {
        g_player.time_acc += dt;
        const float interval = static_cast<float>(1.0 / (g_player.fps > 0.0 ? g_player.fps : 18.2));
        if (g_player.time_acc >= interval) {
            const auto steps = static_cast<std::size_t>(g_player.time_acc / interval);
            g_player.time_acc -= static_cast<float>(steps) * interval;
            if (g_player.current_frame + steps < g_player.frame_files.size()) {
                g_player.current_frame += steps;
            } else if (g_player.looping) {
                g_player.current_frame = (g_player.current_frame + steps) % g_player.frame_files.size();
            } else {
                g_player.current_frame = g_player.frame_files.size() - 1;
                g_player.playing = false;
            }
        }
    }

    ensure_player_frame_texture();

    // 1. Dim background
    DrawRectangle(0, 0, render_width, render_height, backdrop_color);

    // 2. Main panel layout
    const int margin = 24;
    const int panel_w = std::min(render_width - margin * 2, 960);
    const int panel_h = std::min(render_height - margin * 2, 700);
    const int panel_x = (render_width - panel_w) / 2;
    const int panel_y = (render_height - panel_h) / 2;

    DrawRectangle(panel_x, panel_y, panel_w, panel_h, panel_color);
    DrawRectangleLinesEx(Rectangle{static_cast<float>(panel_x), static_cast<float>(panel_y),
                                   static_cast<float>(panel_w), static_cast<float>(panel_h)}, 1.5F, border_color);

    // Header bar
    char title_buf[80];
    std::snprintf(title_buf, sizeof(title_buf), "STARDRIFTER PROJECTOR: MOVIEDECK %s", g_player.deck.deck_str.c_str());
    draw_text_shadow(title_buf, panel_x + 16, panel_y + 14, 18, title_color);

    const double current_time = static_cast<double>(g_player.current_frame) / g_player.fps;
    const double total_time = static_cast<double>(g_player.frame_files.size()) / g_player.fps;
    char time_buf[64];
    std::snprintf(time_buf, sizeof(time_buf), "FRAME %05zu / %05zu  [%02d:%04.1f / %02d:%04.1f]",
                  g_player.current_frame + 1, g_player.frame_files.size(),
                  static_cast<int>(current_time) / 60, std::fmod(current_time, 60.0),
                  static_cast<int>(total_time) / 60, std::fmod(total_time, 60.0));
    const int time_w = MeasureText(time_buf, 14);
    draw_text_shadow(time_buf, panel_x + panel_w - 16 - time_w, panel_y + 16, 14, hint_color);

    DrawLine(panel_x + 12, panel_y + 40, panel_x + panel_w - 12, panel_y + 40, Color{0, 185, 220, 100});

    // 3. Screen area for frame playback
    const int bottom_controls_h = 100;
    const int screen_area_w = panel_w - 32;
    const int screen_area_h = panel_h - 48 - bottom_controls_h;
    const int screen_area_x = panel_x + 16;
    const int screen_area_y = panel_y + 48;

    // Draw frame keeping authentic 4:3 CRT proportions
    if (g_player.texture.id != 0) {
        const float frame_aspect = (g_player.texture.height > 0)
                                       ? static_cast<float>(g_player.texture.width) / static_cast<float>(g_player.texture.height)
                                       : (4.0F / 3.0F);
        float dest_w = static_cast<float>(screen_area_w);
        float dest_h = dest_w / frame_aspect;
        if (dest_h > static_cast<float>(screen_area_h)) {
            dest_h = static_cast<float>(screen_area_h);
            dest_w = dest_h * frame_aspect;
        }

        const float dest_x = static_cast<float>(screen_area_x) + (static_cast<float>(screen_area_w) - dest_w) * 0.5F;
        const float dest_y = static_cast<float>(screen_area_y) + (static_cast<float>(screen_area_h) - dest_h) * 0.5F;

        // Screen bezel
        DrawRectangle(static_cast<int>(dest_x - 4), static_cast<int>(dest_y - 4),
                      static_cast<int>(dest_w + 8), static_cast<int>(dest_h + 8), Color{0, 10, 18, 255});
        DrawRectangleLinesEx(Rectangle{dest_x - 4, dest_y - 4, dest_w + 8, dest_h + 8}, 1.0F, Color{0, 140, 180, 180});

        DrawTexturePro(g_player.texture,
                       Rectangle{0, 0, static_cast<float>(g_player.texture.width), static_cast<float>(g_player.texture.height)},
                       Rectangle{dest_x, dest_y, dest_w, dest_h},
                       Vector2{0, 0}, 0.0F, WHITE);

        // Subtle CRT scanline overlay
        for (int y = static_cast<int>(dest_y); y < static_cast<int>(dest_y + dest_h); y += 3) {
            DrawLine(static_cast<int>(dest_x), y, static_cast<int>(dest_x + dest_w), y, Color{0, 0, 0, 35});
        }
    } else {
        DrawRectangle(screen_area_x, screen_area_y, screen_area_w, screen_area_h, Color{2, 6, 10, 255});
        const char *loading_msg = "LOADING FRAME DATA...";
        const int msg_w = MeasureText(loading_msg, 16);
        draw_text_shadow(loading_msg, screen_area_x + (screen_area_w - msg_w) / 2,
                         screen_area_y + screen_area_h / 2, 16, hint_color);
    }

    // 4. Timeline scrubber bar
    const int scrubber_y = panel_y + panel_h - bottom_controls_h + 8;
    const int scrubber_x = panel_x + 20;
    const int scrubber_w = panel_w - 40;
    const int scrubber_h = 10;

    const Rectangle scrubber_rect{static_cast<float>(scrubber_x), static_cast<float>(scrubber_y),
                                  static_cast<float>(scrubber_w), static_cast<float>(scrubber_h)};

    // Scrubber interaction
    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, scrubber_rect)) {
        g_player.scrubbing = true;
    }
    if (g_player.scrubbing) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const float frac = std::clamp((mouse.x - static_cast<float>(scrubber_x)) / static_cast<float>(scrubber_w), 0.0F, 1.0F);
            g_player.current_frame = static_cast<std::size_t>(frac * static_cast<float>(g_player.frame_files.size() - 1));
            g_player.playing = false;
        } else {
            g_player.scrubbing = false;
        }
    }

    DrawRectangleRec(scrubber_rect, track_bar_color);
    DrawRectangleLinesEx(scrubber_rect, 1.0F, border_color);

    const float progress_frac = (g_player.frame_files.size() > 1)
                                    ? static_cast<float>(g_player.current_frame) / static_cast<float>(g_player.frame_files.size() - 1)
                                    : 1.0F;
    const float played_w = progress_frac * static_cast<float>(scrubber_w);
    DrawRectangle(scrubber_x, scrubber_y, static_cast<int>(played_w), scrubber_h, played_bar_color);

    // Scrubber thumb pip
    const int thumb_x = scrubber_x + static_cast<int>(played_w);
    DrawCircle(thumb_x, scrubber_y + scrubber_h / 2, 7.0F, title_color);
    DrawCircleLines(thumb_x, scrubber_y + scrubber_h / 2, 7.0F, WHITE);

    // 5. Control buttons row
    const int btn_y = scrubber_y + 22;
    const int btn_h = 28;
    int curr_bx = panel_x + 20;

    // [ |< FIRST ]
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 44, static_cast<float>(btn_h)}, "|<", 14)) {
        g_player.current_frame = 0;
        g_player.playing = false;
    }
    curr_bx += 50;

    // [ < STEP ]
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 44, static_cast<float>(btn_h)}, "<", 14)) {
        g_player.playing = false;
        if (g_player.current_frame > 0) --g_player.current_frame;
        else if (g_player.looping) g_player.current_frame = g_player.frame_files.size() - 1;
    }
    curr_bx += 50;

    // [ PLAY / PAUSE ]
    const char *play_label = g_player.playing ? "|| PAUSE" : "> PLAY";
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 86, static_cast<float>(btn_h)}, play_label, 14)) {
        g_player.playing = !g_player.playing;
        show_toast(g_player.playing ? "PLAYING" : "PAUSED", true, 1.2f);
    }
    curr_bx += 92;

    // [ STEP > ]
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 44, static_cast<float>(btn_h)}, ">", 14)) {
        g_player.playing = false;
        if (g_player.current_frame + 1 < g_player.frame_files.size()) ++g_player.current_frame;
        else if (g_player.looping) g_player.current_frame = 0;
    }
    curr_bx += 50;

    // [ LAST >| ]
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 44, static_cast<float>(btn_h)}, ">|", 14)) {
        if (!g_player.frame_files.empty()) {
            g_player.current_frame = g_player.frame_files.size() - 1;
            g_player.playing = false;
        }
    }
    curr_bx += 50;

    // [ LOOP: ON/OFF ]
    const char *loop_label = g_player.looping ? "LOOP: ON" : "LOOP: OFF";
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 88, static_cast<float>(btn_h)}, loop_label, 14)) {
        g_player.looping = !g_player.looping;
        show_toast(g_player.looping ? "LOOP ENABLED" : "LOOP DISABLED", true, 1.5f);
    }
    curr_bx += 94;

    // Speed controls [-] FPS [+]
    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 32, static_cast<float>(btn_h)}, "-", 14)) {
        step_framerate(-1);
    }
    curr_bx += 36;

    char fps_label[32];
    std::snprintf(fps_label, sizeof(fps_label), "%.1f FPS", g_player.fps);
    const int fps_w = MeasureText(fps_label, 14);
    draw_text_shadow(fps_label, curr_bx + (64 - fps_w) / 2, btn_y + 7, 14, text_color);
    curr_bx += 68;

    if (draw_ui_button(Rectangle{static_cast<float>(curr_bx), static_cast<float>(btn_y), 32, static_cast<float>(btn_h)}, "+", 14)) {
        step_framerate(1);
    }

    // Right-aligned action buttons: [ EXPORT VIDEO ] [ FOLDER ] [ CLOSE ]
    const int right_margin = panel_x + panel_w - 20;
    int right_x = right_margin;

    // [ CLOSE ]
    right_x -= 80;
    if (draw_ui_button(Rectangle{static_cast<float>(right_x), static_cast<float>(btn_y), 80, static_cast<float>(btn_h)}, "CLOSE", 14)) {
        close_movie_player();
    }

#ifndef __EMSCRIPTEN__
    // [ FOLDER ]
    right_x -= 88;
    if (draw_ui_button(Rectangle{static_cast<float>(right_x), static_cast<float>(btn_y), 80, static_cast<float>(btn_h)}, "FOLDER", 14)) {
        trigger_movie_open_folder();
    }
#endif

    // [ EXPORT VIDEO ]
    right_x -= 120;
    if (draw_ui_button(Rectangle{static_cast<float>(right_x), static_cast<float>(btn_y), 112, static_cast<float>(btn_h)}, "EXPORT VIDEO", 14)) {
        trigger_movie_export();
    }

    // 6. Toast Notification Banner
    if (g_player.toast_timer > 0.0F) {
        const float alpha = std::min(1.0F, g_player.toast_timer * 2.0F);
        const Color toast_bg = g_player.toast_success ? Color{0, 60, 90, static_cast<unsigned char>(220 * alpha)}
                                                      : Color{90, 20, 20, static_cast<unsigned char>(220 * alpha)};
        const Color toast_border = g_player.toast_success ? Color{0, 210, 240, static_cast<unsigned char>(255 * alpha)}
                                                          : Color{240, 80, 80, static_cast<unsigned char>(255 * alpha)};
        const Color toast_col = g_player.toast_success ? Color{210, 250, 255, static_cast<unsigned char>(255 * alpha)}
                                                       : Color{255, 210, 210, static_cast<unsigned char>(255 * alpha)};

        const int toast_font_size = 15;
        const int toast_w = MeasureText(g_player.toast_text.c_str(), toast_font_size) + 36;
        const int toast_h = 32;
        const int toast_x = panel_x + (panel_w - toast_w) / 2;
        const int toast_y = panel_y + 54;

        DrawRectangle(toast_x, toast_y, toast_w, toast_h, toast_bg);
        DrawRectangleLinesEx(Rectangle{static_cast<float>(toast_x), static_cast<float>(toast_y),
                                       static_cast<float>(toast_w), static_cast<float>(toast_h)}, 1.0F, toast_border);
        const int text_x = toast_x + (toast_w - MeasureText(g_player.toast_text.c_str(), toast_font_size)) / 2;
        const int text_y = toast_y + (toast_h - toast_font_size) / 2;
        draw_text_shadow(g_player.toast_text.c_str(), text_x, text_y, toast_font_size, toast_col);
    }
}

} // namespace noctis
