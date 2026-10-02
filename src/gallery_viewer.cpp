#include "gallery_viewer.h"

#include "gallery.h"

#include <raylib.h>

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
} // namespace

bool open_gallery_viewer(const std::filesystem::path &directory, std::string_view key) {
    auto scanned = scan_gallery(directory);
    const auto index = find_gallery_entry(scanned, key);
    if (!index) return false;
    release_texture();
    entries = std::move(scanned);
    state = GalleryViewerState{true, *index, entries.size(), false, 0.5F};
    escape_latched = false;
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
            release_texture();
            break;
        }
    }
    return true;
}

void render_gallery_viewer(int render_width, int render_height, const DisplayViewport &viewport) {
    if (!state.open || entries.empty()) return;
    ensure_texture();
    const auto &entry = entries[state.index];

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
    draw_text_shadowed(counter, right - MeasureText(counter, font), header_y, font, text_color);

    char info[96];
    std::snprintf(info, sizeof(info), "%s.BMP  %s %dX%d", entry.id.c_str(), gallery_kind_name(entry.kind),
                  entry.width, entry.height);
    draw_text_shadowed(info, left, footer_y, font, text_color);
    const char *hints = state.zoomed ? "LEFT/RIGHT PAN   PGUP/PGDN   Z FIT   ESC CLOSE"
                                     : "LEFT/RIGHT BROWSE   HOME/END   Z ZOOM   ESC CLOSE";
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
}

void shutdown_gallery_viewer() {
    release_texture();
    state = {};
    entries.clear();
}

} // namespace noctis
