#include "gallery.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>

namespace noctis {
namespace {
constexpr std::size_t file_header_size = 14;
constexpr std::size_t info_header_size = 40;
constexpr std::int32_t maximum_dimension = 4096;

std::uint32_t read_u32(const std::uint8_t *bytes) {
    return static_cast<std::uint32_t>(bytes[0]) | static_cast<std::uint32_t>(bytes[1]) << 8
        | static_cast<std::uint32_t>(bytes[2]) << 16 | static_cast<std::uint32_t>(bytes[3]) << 24;
}

std::uint16_t read_u16(const std::uint8_t *bytes) {
    return static_cast<std::uint16_t>(bytes[0] | bytes[1] << 8);
}

struct BmpLayout {
    std::int32_t width{};
    std::int32_t height{};
    bool top_down{};
    std::size_t palette_offset{};
    std::size_t palette_entries{};
    std::size_t pixel_offset{};
    std::size_t stride{};
};

std::optional<BmpLayout> parse_layout(const std::uint8_t *bytes, std::size_t size) {
    if (size < file_header_size + info_header_size || bytes[0] != 'B' || bytes[1] != 'M') return std::nullopt;
    const auto info_size = read_u32(bytes + 14);
    const auto raw_height = static_cast<std::int32_t>(read_u32(bytes + 22));
    BmpLayout layout;
    layout.width = static_cast<std::int32_t>(read_u32(bytes + 18));
    layout.top_down = raw_height < 0;
    layout.height = layout.top_down ? -raw_height : raw_height;
    if (info_size < info_header_size || read_u16(bytes + 26) != 1 || read_u16(bytes + 28) != 8
        || read_u32(bytes + 30) != 0 || layout.width <= 0 || layout.height <= 0
        || layout.width > maximum_dimension || layout.height > maximum_dimension)
        return std::nullopt;
    const auto colors_used = read_u32(bytes + 46);
    layout.palette_entries = colors_used == 0 || colors_used > 256 ? 256 : colors_used;
    layout.palette_offset = file_header_size + info_size;
    layout.pixel_offset = read_u32(bytes + 10);
    layout.stride = (static_cast<std::size_t>(layout.width) + 3) & ~std::size_t{3};
    return layout;
}

GalleryImageKind classify(std::int32_t width, std::int32_t height) {
    if (width == 320 && height == 200) return GalleryImageKind::snapshot;
    if (width >= 2 * height) return GalleryImageKind::panorama;
    return GalleryImageKind::other;
}

bool all_digits(std::string_view text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; });
}

std::uint32_t parse_number(std::string_view digits) {
    std::uint32_t value = 0;
    for (const char c : digits) value = value * 10 + static_cast<std::uint32_t>(c - '0');
    return value;
}

std::string uppercase(std::string_view text) {
    std::string result(text);
    for (char &c : result)
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - ('a' - 'A'));
    return result;
}
} // namespace

std::vector<GalleryEntry> scan_gallery(const std::filesystem::path &directory) {
    std::vector<GalleryEntry> entries;
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, error);
    if (error) return entries;
    for (const auto &item : iterator) {
        if (!item.is_regular_file(error) || error) continue;
        const auto stem = uppercase(item.path().stem().string());
        if (uppercase(item.path().extension().string()) != ".BMP") continue;

        GalleryEntry entry;
        if (stem.size() == 8 && all_digits(stem)) {
            entry.number = parse_number(stem);
        } else if (stem.size() == 8 && stem.starts_with("SNAP") && all_digits(std::string_view(stem).substr(4))) {
            entry.number = parse_number(std::string_view(stem).substr(4));
            entry.legacy = true;
        } else {
            continue; // Includes WIDE9997-9999 panorama temporaries.
        }

        std::array<std::uint8_t, file_header_size + info_header_size> header{};
        std::ifstream input(item.path(), std::ios::binary);
        if (!input.read(reinterpret_cast<char *>(header.data()), header.size())) continue;
        const auto layout = parse_layout(header.data(), header.size());
        if (!layout) continue;

        entry.path = item.path();
        entry.id = stem;
        entry.width = layout->width;
        entry.height = layout->height;
        entry.kind = classify(layout->width, layout->height);
        entries.push_back(std::move(entry));
    }
    std::sort(entries.begin(), entries.end(), [](const GalleryEntry &left, const GalleryEntry &right) {
        if (left.legacy != right.legacy) return left.legacy;
        return left.number < right.number;
    });
    return entries;
}

std::optional<GalleryImage> load_gallery_image(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;
    const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) return std::nullopt;
    const auto layout = parse_layout(bytes.data(), bytes.size());
    if (!layout || layout->palette_offset + layout->palette_entries * 4 > bytes.size()
        || layout->pixel_offset > bytes.size()
        || layout->stride * static_cast<std::size_t>(layout->height) > bytes.size() - layout->pixel_offset)
        return std::nullopt;

    // Out-of-range indices render black rather than reading past the palette.
    std::array<std::array<std::uint8_t, 4>, 256> palette{};
    for (std::size_t color = 0; color < layout->palette_entries; ++color) {
        const auto *bgra = bytes.data() + layout->palette_offset + color * 4;
        palette[color] = {bgra[2], bgra[1], bgra[0], 255};
    }
    for (std::size_t color = layout->palette_entries; color < palette.size(); ++color) palette[color] = {0, 0, 0, 255};

    GalleryImage image;
    image.width = layout->width;
    image.height = layout->height;
    image.rgba.resize(static_cast<std::size_t>(image.width) * image.height * 4);
    for (std::int32_t row = 0; row < image.height; ++row) {
        const auto source_row = layout->top_down ? row : image.height - 1 - row;
        const auto *source = bytes.data() + layout->pixel_offset + layout->stride * static_cast<std::size_t>(source_row);
        auto *target = image.rgba.data() + static_cast<std::size_t>(row) * image.width * 4;
        for (std::int32_t column = 0; column < image.width; ++column) {
            std::copy_n(palette[source[column]].begin(), 4, target + static_cast<std::size_t>(column) * 4);
        }
    }
    return image;
}

std::optional<std::size_t> find_gallery_entry(const std::vector<GalleryEntry> &entries, std::string_view key) {
    if (entries.empty()) return std::nullopt;
    if (key.empty()) return entries.size() - 1;
    if (all_digits(key) && key.size() <= 8) {
        const auto number = parse_number(key);
        for (std::size_t index = entries.size(); index-- > 0;)
            if (!entries[index].legacy && entries[index].number == number) return index;
        return std::nullopt;
    }
    const auto wanted = uppercase(key);
    for (std::size_t index = 0; index < entries.size(); ++index)
        if (entries[index].id == wanted) return index;
    return std::nullopt;
}

const char *gallery_kind_name(GalleryImageKind kind) {
    switch (kind) {
    case GalleryImageKind::snapshot: return "SNAPSHOT";
    case GalleryImageKind::panorama: return "PANORAMA";
    case GalleryImageKind::other: break;
    }
    return "IMAGE";
}

std::vector<GalleryCommand> gallery_commands_for_frame(const InputFrame &frame, bool zoomed) {
    std::vector<GalleryCommand> commands;
    if (frame.escape_down || frame.enter_pressed || frame.f4_pressed) {
        commands.push_back(GalleryCommand::close);
        return commands;
    }
    if (frame.arrow_left_pressed) commands.push_back(zoomed ? GalleryCommand::pan_left : GalleryCommand::previous);
    if (frame.arrow_right_pressed) commands.push_back(zoomed ? GalleryCommand::pan_right : GalleryCommand::next);
    if (frame.arrow_up_pressed || frame.page_up_pressed) commands.push_back(GalleryCommand::previous);
    if (frame.arrow_down_pressed || frame.page_down_pressed) commands.push_back(GalleryCommand::next);
    if (frame.home_pressed) commands.push_back(GalleryCommand::first);
    if (frame.end_pressed) commands.push_back(GalleryCommand::last);
    const bool zoom_text = std::any_of(frame.text.begin(), frame.text.end(),
                                       [](std::int32_t key) { return key == 'z' || key == 'Z' || key == ' '; });
    if (zoom_text || frame.space_pressed) commands.push_back(GalleryCommand::toggle_zoom);
    return commands;
}

bool apply_gallery_command(GalleryViewerState &state, GalleryCommand command) {
    if (!state.open || state.count == 0) {
        state.open = false;
        return false;
    }
    const auto previous_index = state.index;
    switch (command) {
    case GalleryCommand::none: return false;
    case GalleryCommand::close: state.open = false; return false;
    case GalleryCommand::previous: if (state.index > 0) --state.index; break;
    case GalleryCommand::next: if (state.index + 1 < state.count) ++state.index; break;
    case GalleryCommand::first: state.index = 0; break;
    case GalleryCommand::last: state.index = state.count - 1; break;
    case GalleryCommand::toggle_zoom:
        state.zoomed = !state.zoomed;
        state.pan = 0.5F;
        return false;
    case GalleryCommand::pan_left:
        if (state.zoomed) state.pan = std::max(0.0F, state.pan - gallery_pan_step);
        return false;
    case GalleryCommand::pan_right:
        if (state.zoomed) state.pan = std::min(1.0F, state.pan + gallery_pan_step);
        return false;
    }
    if (state.index == previous_index) return false;
    state.pan = 0.5F;
    return true;
}

} // namespace noctis
