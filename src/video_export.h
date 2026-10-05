#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

enum class VideoFormat : std::uint8_t {
    mp4,
    webm,
};

struct MovieDeckEntry {
    std::uint16_t deck = 1;
    std::string deck_str; // e.g. "001"
    std::filesystem::path path;
    std::size_t frame_count = 0;
    std::int32_t width = 320;
    std::int32_t height = 200;
    double fps = 18.2;
};

// Discovers and inventories all recorded movie decks in the given movies directory.
std::vector<MovieDeckEntry> scan_movie_decks(const std::filesystem::path &movies_dir);

struct VideoExportOptions {
    std::filesystem::path deck_dir;
    std::filesystem::path output_path; // If empty, automatically placed in user_downloads_directory()
    double fps = 18.2;
    VideoFormat format = VideoFormat::mp4;
};

struct VideoExportStatus {
    bool in_progress = false;
    bool finished = false;
    bool success = false;
    std::string message;
    std::filesystem::path output_file;
};

// Returns whether ffmpeg is installed and accessible on the host system.
bool is_ffmpeg_available(std::string *version_out = nullptr);

// Searches for ffmpeg executable in PATH or next to the game binary.
std::filesystem::path find_ffmpeg_binary();

// Constructs the exact CLI command string for invoking ffmpeg.
std::string build_ffmpeg_command(const VideoExportOptions &options, const std::filesystem::path &ffmpeg_bin = {});

// Synchronously exports a movie deck using ffmpeg. Safe for CLI usage or tests.
VideoExportStatus export_video_sync(const VideoExportOptions &options);

// Asynchronously exports a movie deck in a background worker thread.
bool start_video_export_async(const VideoExportOptions &options);
bool is_video_export_running();
VideoExportStatus get_video_export_status();
void reset_video_export_status();

#if defined(__EMSCRIPTEN__)
// HTML5 MediaRecorder browser video recording (M14-W01)
bool browser_media_recorder_supported();
bool start_browser_video_recording(int fps = 20);
bool stop_browser_video_recording(const char *deck_name = "noctis_movie");
bool is_browser_video_recording();
#endif

} // namespace noctis
