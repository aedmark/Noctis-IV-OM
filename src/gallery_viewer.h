#pragma once

#include "display.h"
#include "input.h"

#include <filesystem>
#include <string_view>

namespace noctis {

// Opens the cockpit image viewer on the entry named by key (empty selects the
// newest image). Returns false when the gallery has no matching image.
bool open_gallery_viewer(const std::filesystem::path &directory, std::string_view key);
bool gallery_viewer_open();

// Overlay input handler: consumes every frame while the viewer is open, and
// the held Escape that closed it.
bool gallery_viewer_input(const InputFrame &frame);

// Draws the viewer at full window resolution above the presented frame.
void render_gallery_viewer(int render_width, int render_height, const DisplayViewport &viewport);

// Releases the GPU texture; call before closing the window.
void shutdown_gallery_viewer();

} // namespace noctis
