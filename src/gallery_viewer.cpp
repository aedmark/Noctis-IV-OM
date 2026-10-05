#include "gallery_viewer.h"

#include "audio.h"
#include "gallery.h"

#include <raylib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

namespace noctis {
namespace {
constexpr std::size_t no_texture = std::numeric_limits<std::size_t>::max();
constexpr Color panel_color{6, 10, 16, 235};
constexpr Color border_color{0, 185, 220, 200};
constexpr Color title_color{0, 230, 195, 255};
constexpr Color text_color{235, 245, 255, 255};
constexpr Color hint_color{150, 170, 190, 255};

GalleryViewerState state;
std::vector<GalleryEntry> entries;
Texture2D texture{};
std::size_t texture_index = no_texture;
bool load_failed{};
bool escape_latched{};

std::string toast_text;
float toast_timer{};
bool toast_success{true};

void release_texture() {
    if (texture.id != 0) UnloadTexture(texture);
    texture = {};
    texture_index = no_texture;
}

void ensure_texture() {
    if (texture_index == state.index) return;
    release_texture();
    texture_index = state.index;
    auto decoded = load_gallery_image(entries[state.index].path);
    load_failed = !decoded;
    if (load_failed) return;
    const Image image{decoded->rgba.data(), decoded->width, decoded->height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    texture = LoadTextureFromImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    load_failed = texture.id == 0;
}

void draw_text_shadowed(const char *text, int x, int y, int size, Color color) {
    DrawText(text, x + 1, y + 1, size, Color{0, 0, 0, 180});
    DrawText(text, x, y, size, color);
}

void close_gallery_viewer() {
    if (!state.open) return;
    state.open = false;
    release_texture();
    toast_timer = 0.0F;
#ifndef __EMSCRIPTEN__
    if (IsWindowReady() && is_cursor_lock_wanted()) {
        DisableCursor();
    }
#endif
}

void trigger_export(std::optional<GalleryExportFormat> forced_fmt = std::nullopt) {
    if (entries.empty() || state.index >= entries.size()) return;
    const auto &entry = entries[state.index];
    const auto fmt = forced_fmt.value_or(state.export_format);
    const bool ok = export_gallery_image(entry, std::nullopt, fmt);
    play_goesnet_chime(ok);
    toast_success = ok;
    toast_timer = 3.5F;
    const std::string ext = (fmt == GalleryExportFormat::png) ? ".PNG" : ".BMP";
    if (ok) {
#ifdef __EMSCRIPTEN__
        toast_text = "DOWNLOADING " + entry.id + ext;
#else
        toast_text = "EXPORTED " + entry.id + ext + " TO DOWNLOADS";
#endif
    } else {
        toast_text = "EXPORT FAILED";
    }
}

void trigger_open_folder() {
#ifndef __EMSCRIPTEN__
    if (entries.empty() || state.index >= entries.size()) return;
    const auto &entry = entries[state.index];
    const bool ok = open_gallery_folder(entry.path.parent_path());
    play_goesnet_chime(ok);
    toast_success = ok;
    toast_timer = 3.5F;
    toast_text = ok ? "OPENED GALLERY FOLDER" : "COULD NOT OPEN FOLDER";
#endif
}

bool draw_button(const Rectangle &rect, const char *label, int font_size) {
    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, rect);
    const bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    const Color bg = hovered ? Color{0, 185, 220, 55} : Color{12, 22, 34, 210};
    const Color border = hovered ? title_color : border_color;
    const Color text_col = hovered ? title_color : text_color;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 1.0F, border);
    const int text_w = MeasureText(label, font_size);
    const int text_x = static_cast<int>(rect.x + (rect.width - static_cast<float>(text_w)) * 0.5F);
    const int text_y = static_cast<int>(rect.y + (rect.height - static_cast<float>(font_size)) * 0.5F);
    draw_text_shadowed(label, text_x, text_y, font_size, text_col);
    return clicked;
}
} // namespace

bool open_gallery_viewer(const std::filesystem::path &directory, std::string_view key) {
    auto scanned = scan_gallery(directory);
    const auto index = find_gallery_entry(scanned, key);
    if (!index) return false;
    release_texture();
    entries = std::move(scanned);
    state = GalleryViewerState{true, *index, entries.size(), false, 0.5F};
    escape_latched = false;
    toast_text.clear();
    toast_timer = 0.0F;

#ifndef __EMSCRIPTEN__
    if (IsWindowReady()) {
        EnableCursor();
    }
#else
    EM_ASM({
        if (document.exitPointerLock) {
            document.exitPointerLock();
        }
    });
#endif

    return true;
}

bool gallery_viewer_open() { return state.open; }

bool gallery_viewer_input(const InputFrame &frame) {
    if (escape_latched) {
        if (frame.escape_down) return true;
        escape_latched = false;
    }
    if (!state.open) return false;
    for (const auto command : gallery_commands_for_frame(frame, state.zoomed)) {
        apply_gallery_command(state, command);
        if (command == GalleryCommand::close) {
            escape_latched = frame.escape_down;
            close_gallery_viewer();
            break;
        }
        if (command == GalleryCommand::download) {
            trigger_export();
        }
        if (command == GalleryCommand::export_png) {
            trigger_export(GalleryExportFormat::png);
        }
        if (command == GalleryCommand::export_bmp) {
            trigger_export(GalleryExportFormat::bmp);
        }
        if (command == GalleryCommand::toggle_format) {
            toast_success = true;
            toast_timer = 2.0F;
            toast_text = (state.export_format == GalleryExportFormat::png) ? "EXPORT FORMAT: PNG" : "EXPORT FORMAT: BMP";
        }
        if (command == GalleryCommand::open_folder) {
            trigger_open_folder();
        }
    }
    return true;
}

void render_gallery_viewer(int render_width, int render_height, const DisplayViewport &viewport) {
    if (!state.open || entries.empty()) return;
    ensure_texture();
    const auto &entry = entries[state.index];

    if (toast_timer > 0.0F) {
        toast_timer -= GetFrameTime();
        if (toast_timer < 0.0F) toast_timer = 0.0F;
    }

    DrawRectangle(0, 0, render_width, render_height, Color{2, 4, 8, 215});

    const float margin = std::max(8.0F, viewport.height * 0.03F);
    const int font = std::clamp(static_cast<int>(viewport.height * 0.028F), 12, 26);
    const float bar = static_cast<float>(font) * 1.8F;
    const Rectangle panel{viewport.x + margin, viewport.y + margin, viewport.width - 2 * margin,
                          viewport.height - 2 * margin};
    DrawRectangleRec(panel, panel_color);
    DrawRectangleLinesEx(panel, 1.5F, border_color);
    DrawLineEx({panel.x, panel.y + bar}, {panel.x + panel.width, panel.y + bar}, 1.0F, border_color);
    DrawLineEx({panel.x, panel.y + panel.height - bar}, {panel.x + panel.width, panel.y + panel.height - bar}, 1.0F,
               border_color);

    const int pad = font;
    const int header_y = static_cast<int>(panel.y + (bar - static_cast<float>(font)) * 0.5F);
    const int footer_y = static_cast<int>(panel.y + panel.height - bar + (bar - static_cast<float>(font)) * 0.5F);
    const int left = static_cast<int>(panel.x) + pad;
    const int right = static_cast<int>(panel.x + panel.width) - pad;

    draw_text_shadowed("GOES IMAGE ARCHIVE", left, header_y, font, title_color);

    char counter[48];
    std::snprintf(counter, sizeof(counter), "IMAGE %zu OF %zu", state.index + 1, state.count);
    const int counter_w = MeasureText(counter, font);
    const int counter_x = right - counter_w;
    draw_text_shadowed(counter, counter_x, header_y, font, text_color);

    // Interactive buttons in header
    const int btn_font = std::max(10, font * 4 / 5);
    const float btn_h = bar * 0.72F;
    const float btn_y = panel.y + (bar - btn_h) * 0.5F;
    const float btn_pad_x = static_cast<float>(font) * 0.5F;

    const bool is_png = (state.export_format == GalleryExportFormat::png);
#ifdef __EMSCRIPTEN__
    const char *export_label = is_png ? "⬇ DOWNLOAD PNG (D)" : "⬇ DOWNLOAD BMP (D)";
#else
    const char *export_label = is_png ? "⬇ EXPORT PNG (D)" : "⬇ EXPORT BMP (D)";
    const char *folder_label = "📁 FOLDER (O)";
#endif
    const char *fmt_label = is_png ? "PNG (F)" : "BMP (F)";
    const char *close_label = "✕ CLOSE (ESC)";

    const float exp_w = MeasureText(export_label, btn_font) + btn_pad_x * 2.0F;
    const float fmt_w = MeasureText(fmt_label, btn_font) + btn_pad_x * 2.0F;
#ifndef __EMSCRIPTEN__
    const float fld_w = MeasureText(folder_label, btn_font) + btn_pad_x * 2.0F;
#endif
    const float cls_w = MeasureText(close_label, btn_font) + btn_pad_x * 2.0F;

    const float title_end_x = static_cast<float>(left + MeasureText("GOES IMAGE ARCHIVE", font)) + 16.0F;
    const float counter_start_x = static_cast<float>(counter_x) - 16.0F;
    const float header_mid_space = counter_start_x - title_end_x;

#ifdef __EMSCRIPTEN__
    const float total_btns_w = exp_w + 8.0F + fmt_w + 8.0F + cls_w;
#else
    const float total_btns_w = exp_w + 8.0F + fmt_w + 8.0F + fld_w + 8.0F + cls_w;
#endif

    if (header_mid_space >= total_btns_w) {
        float cur_x = title_end_x + (header_mid_space - total_btns_w) * 0.5F;
        if (draw_button({cur_x, btn_y, exp_w, btn_h}, export_label, btn_font)) {
            trigger_export();
        }
        cur_x += exp_w + 8.0F;
        if (draw_button({cur_x, btn_y, fmt_w, btn_h}, fmt_label, btn_font)) {
            state.export_format = is_png ? GalleryExportFormat::bmp : GalleryExportFormat::png;
            toast_success = true;
            toast_timer = 2.0F;
            toast_text = (state.export_format == GalleryExportFormat::png) ? "EXPORT FORMAT: PNG" : "EXPORT FORMAT: BMP";
        }
        cur_x += fmt_w + 8.0F;
#ifndef __EMSCRIPTEN__
        if (draw_button({cur_x, btn_y, fld_w, btn_h}, folder_label, btn_font)) {
            trigger_open_folder();
        }
        cur_x += fld_w + 8.0F;
#endif
        if (draw_button({cur_x, btn_y, cls_w, btn_h}, close_label, btn_font)) {
            close_gallery_viewer();
        }
    } else if (header_mid_space >= exp_w + 8.0F + cls_w) {
        float cur_x = title_end_x + (header_mid_space - (exp_w + 8.0F + cls_w)) * 0.5F;
        if (draw_button({cur_x, btn_y, exp_w, btn_h}, export_label, btn_font)) {
            trigger_export();
        }
        cur_x += exp_w + 8.0F;
        if (draw_button({cur_x, btn_y, cls_w, btn_h}, close_label, btn_font)) {
            close_gallery_viewer();
        }
    }

    // Previous / Next buttons next to counter if multiple images
    if (state.count > 1) {
        const float nav_w = static_cast<float>(btn_font) * 1.8F;
        if (counter_start_x - nav_w - 4.0F > title_end_x + total_btns_w) {
            if (draw_button({static_cast<float>(counter_x) - nav_w - 8.0F, btn_y, nav_w, btn_h}, "<", btn_font)) {
                apply_gallery_command(state, GalleryCommand::previous);
            }
        }
    }

    char info[96];
    std::snprintf(info, sizeof(info), "%s.BMP  %s %dX%d", entry.id.c_str(), gallery_kind_name(entry.kind),
                  entry.width, entry.height);
    draw_text_shadowed(info, left, footer_y, font, text_color);

#ifdef __EMSCRIPTEN__
    const char *hints = state.zoomed ? "LEFT/RIGHT PAN   D DOWNLOAD   F FORMAT   Z FIT   ESC CLOSE"
                                     : "LEFT/RIGHT BROWSE   D DOWNLOAD   F FORMAT   Z ZOOM   ESC CLOSE";
#else
    const char *hints = state.zoomed ? "LEFT/RIGHT PAN   D EXPORT   F FORMAT   O FOLDER   Z FIT   ESC CLOSE"
                                     : "LEFT/RIGHT BROWSE   D EXPORT   F FORMAT   O FOLDER   Z ZOOM   ESC CLOSE";
#endif

    const int hint_font = std::max(10, font * 4 / 5);
    const int hint_width = MeasureText(hints, hint_font);
    if (left + MeasureText(info, font) + pad * 2 + hint_width <= right) {
        draw_text_shadowed(hints, right - hint_width, footer_y + (font - hint_font) / 2, hint_font, hint_color);
    }

    const Rectangle area{panel.x + static_cast<float>(pad), panel.y + bar + static_cast<float>(pad),
                         panel.width - 2.0F * static_cast<float>(pad), panel.height - 2.0F * bar - 2.0F * static_cast<float>(pad)};
    if (area.width <= 0.0F || area.height <= 0.0F) return;

    if (load_failed) {
        const char *message = "IMAGE DATA CORRUPT";
        draw_text_shadowed(message, static_cast<int>(area.x + (area.width - MeasureText(message, font)) * 0.5F),
                           static_cast<int>(area.y + (area.height - font) * 0.5F), font, Color{255, 120, 90, 255});
        return;
    }

    // Keep the same pixel aspect the live 320x200 frame has in this viewport.
    const float pixel_aspect = (viewport.width / 320.0F) / (viewport.height / 200.0F);
    const float width = static_cast<float>(texture.width);
    const float height = static_cast<float>(texture.height);
    const float scale = state.zoomed ? area.height / height
                                     : std::min(area.width / (width * pixel_aspect), area.height / height);
    const float shown_width = width * pixel_aspect * scale;
    const float shown_height = height * scale;
    const float y = area.y + (area.height - shown_height) * 0.5F;

    Rectangle source{0.0F, 0.0F, width, height};
    Rectangle target{area.x + (area.width - shown_width) * 0.5F, y, shown_width, shown_height};
    const bool panning = shown_width > area.width;
    if (panning) {
        source.width = area.width / (pixel_aspect * scale);
        source.x = state.pan * (width - source.width);
        target = {area.x, y, area.width, shown_height};
    }
    DrawTexturePro(texture, source, target, {0.0F, 0.0F}, 0.0F, WHITE);

    // Faint phosphor scanlines once each source row is tall enough to show them.
    if (scale >= 3.0F) {
        const int line = std::max(1, static_cast<int>(scale * 0.25F));
        for (int row = 0; row < texture.height; ++row) {
            DrawRectangle(static_cast<int>(target.x), static_cast<int>(y + (static_cast<float>(row) + 1.0F) * scale) - line,
                          static_cast<int>(target.width), line, Color{0, 0, 0, 45});
        }
    }
    DrawRectangleLinesEx({target.x - 1.0F, target.y - 1.0F, target.width + 2.0F, target.height + 2.0F}, 1.0F,
                         Color{0, 185, 220, 90});

    if (panning) {
        const float track_y = std::min(target.y + target.height + static_cast<float>(pad) * 0.5F,
                                       panel.y + panel.height - bar - 4.0F);
        const Rectangle track{area.x, track_y, area.width, 3.0F};
        const float thumb_width = track.width * (source.width / width);
        DrawRectangleRec(track, Color{0, 185, 220, 60});
        DrawRectangleRec({track.x + state.pan * (track.width - thumb_width), track.y, thumb_width, track.height},
                         title_color);
    }

    // Toast notification overlay banner
    if (toast_timer > 0.0F && !toast_text.empty()) {
        const float alpha = toast_timer < 0.5F ? (toast_timer / 0.5F) : 1.0F;
        const int toast_font = font;
        const int toast_tw = MeasureText(toast_text.c_str(), toast_font);
        const float toast_w = static_cast<float>(toast_tw) + 40.0F;
        const float toast_h = static_cast<float>(toast_font) * 1.8F;
        const float toast_x = panel.x + (panel.width - toast_w) * 0.5F;
        const float toast_y = panel.y + bar + 14.0F;

        const Color toast_bg = toast_success
            ? Color{8, 30, 26, static_cast<unsigned char>(235 * alpha)}
            : Color{38, 12, 10, static_cast<unsigned char>(235 * alpha)};
        const Color toast_border = toast_success
            ? Color{0, 230, 195, static_cast<unsigned char>(255 * alpha)}
            : Color{255, 120, 90, static_cast<unsigned char>(255 * alpha)};
        const Color toast_text_col = toast_success
            ? Color{235, 255, 250, static_cast<unsigned char>(255 * alpha)}
            : Color{255, 220, 210, static_cast<unsigned char>(255 * alpha)};

        DrawRectangleRec({toast_x, toast_y, toast_w, toast_h}, toast_bg);
        DrawRectangleLinesEx({toast_x, toast_y, toast_w, toast_h}, 1.2F, toast_border);
        const int tx = static_cast<int>(toast_x + 20.0F);
        const int ty = static_cast<int>(toast_y + (toast_h - static_cast<float>(toast_font)) * 0.5F);
        DrawText(toast_text.c_str(), tx + 1, ty + 1, toast_font,
                 Color{0, 0, 0, static_cast<unsigned char>(180 * alpha)});
        DrawText(toast_text.c_str(), tx, ty, toast_font, toast_text_col);
    }
}

void shutdown_gallery_viewer() {
    close_gallery_viewer();
    state = {};
    entries.clear();
    toast_text.clear();
    toast_timer = 0.0F;
}

} // namespace noctis
