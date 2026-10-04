#pragma once

#include "input.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

enum class GalleryImageKind : std::uint8_t { snapshot, panorama, other };

// One viewable image in the player's gallery directory. Entries are ordered
// chronologically: legacy SNAPnnnn names first, then eight-digit NIV+ names.
struct GalleryEntry {
    std::filesystem::path path;
    std::string id; // File stem as shown on the GOES console, e.g. "00000042".
    std::uint32_t number{};
    bool legacy{};
    std::int32_t width{};
    std::int32_t height{};
    GalleryImageKind kind = GalleryImageKind::other;
};

struct GalleryImage {
    std::int32_t width{};
    std::int32_t height{};
    std::vector<std::uint8_t> rgba;
};

// Lists every readable 8-bit snapshot or panorama BMP. Panorama temporaries
// (WIDE9997-9999) and malformed files are skipped.
std::vector<GalleryEntry> scan_gallery(const std::filesystem::path &directory);

// Decodes an uncompressed 8-bit indexed BMP with its own palette.
std::optional<GalleryImage> load_gallery_image(const std::filesystem::path &path);

// Resolves a console key: empty selects the newest entry; digits match the
// sequence number; anything else matches the file stem.
std::optional<std::size_t> find_gallery_entry(const std::vector<GalleryEntry> &entries, std::string_view key);

const char *gallery_kind_name(GalleryImageKind kind);

enum class GalleryCommand : std::uint8_t {
    none,
    close,
    previous,
    next,
    first,
    last,
    toggle_zoom,
    pan_left,
    pan_right,
    download,
    open_folder,
};

// Returns the user's standard Downloads directory.
std::filesystem::path user_downloads_directory();

// Exports or downloads the given gallery image:
// - On Web (Emscripten): triggers an HTML5 Blob download via a virtual link click.
// - On Desktop: copies the BMP to destination_override if set, or user's Downloads directory.
// Returns true on success, false on failure.
bool export_gallery_image(const GalleryEntry &entry,
                          const std::optional<std::filesystem::path> &destination_override = std::nullopt);

// Opens the specified directory (or file's parent folder) in the system file explorer:
// - On Windows: explorer.exe / ShellExecute.
// - On Linux: xdg-open.
// - On Web: no-op (returns false).
// Returns true if successfully launched.
bool open_gallery_folder(const std::filesystem::path &path);

// Browsing state for the cockpit image viewer. Pan is the left edge of the
// visible window as a fraction of the image's horizontal travel (0..1).
struct GalleryViewerState {
    bool open{};
    std::size_t index{};
    std::size_t count{};
    bool zoomed{};
    float pan{};
};

constexpr float gallery_pan_step = 0.125F;

// Maps one polled frame to viewer commands. While zoomed, Left/Right pan and
// Page Up/Down, Up/Down still browse.
std::vector<GalleryCommand> gallery_commands_for_frame(const InputFrame &frame, bool zoomed);

// Applies one command and reports whether the displayed image changed.
bool apply_gallery_command(GalleryViewerState &state, GalleryCommand command);

} // namespace noctis
