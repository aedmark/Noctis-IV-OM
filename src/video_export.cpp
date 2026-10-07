#include "video_export.h"

#include "gallery.h"
#include "runtime_paths.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif

namespace noctis {

namespace {

std::mutex g_export_mutex;
VideoExportStatus g_export_status;

bool is_digit_char(char c) {
    return c >= '0' && c <= '9';
}

std::uint32_t parse_frame_number(std::string_view stem) {
    std::uint32_t val = 0;
    for (char c : stem) {
        if (is_digit_char(c)) {
            val = val * 10 + static_cast<std::uint32_t>(c - '0');
        }
    }
    return val;
}

std::pair<std::size_t, std::uint32_t> inspect_deck_frames(const std::filesystem::path &deck_dir) {
    std::size_t count = 0;
    std::uint32_t min_frame = 1;
    bool found_first = false;

    std::error_code ec;
    if (!std::filesystem::is_directory(deck_dir, ec) || ec) {
        return {0, 1};
    }

    for (const auto &entry : std::filesystem::directory_iterator(deck_dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec) || ec) continue;
        const auto ext = entry.path().extension().string();
        if (ext == ".BMP" || ext == ".bmp") {
            const auto stem = entry.path().stem().string();
            const auto num = parse_frame_number(stem);
            if (num > 0) {
                if (!found_first || num < min_frame) {
                    min_frame = num;
                    found_first = true;
                }
                ++count;
            }
        }
    }
    return {count, min_frame};
}

} // namespace

std::vector<MovieDeckEntry> scan_movie_decks(const std::filesystem::path &movies_dir) {
    std::vector<MovieDeckEntry> decks;
    std::error_code ec;
    if (!std::filesystem::is_directory(movies_dir, ec) || ec) {
        return decks;
    }

    for (const auto &item : std::filesystem::directory_iterator(movies_dir, ec)) {
        if (ec) break;
        if (!item.is_directory(ec) || ec) continue;

        const auto stem = item.path().filename().string();
        if (stem.empty()) continue;

        bool all_digits = true;
        for (char c : stem) {
            if (c < '0' || c > '9') { all_digits = false; break; }
        }
        if (!all_digits) continue;

        const auto num = static_cast<std::uint16_t>(std::strtoul(stem.c_str(), nullptr, 10));
        if (num == 0) continue;

        std::size_t frame_count = 0;
        std::filesystem::path sample_frame;
        for (const auto &sub : std::filesystem::directory_iterator(item.path(), ec)) {
            if (ec) break;
            if (!sub.is_regular_file(ec) || ec) continue;
            const auto ext = sub.path().extension().string();
            if (ext == ".BMP" || ext == ".bmp") {
                ++frame_count;
                if (sample_frame.empty()) sample_frame = sub.path();
            }
        }

        if (frame_count > 0) {
            MovieDeckEntry entry;
            entry.deck = num;
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%03u", num);
            entry.deck_str = buf;
            entry.path = item.path();
            entry.frame_count = frame_count;
            entry.fps = 18.2;

            if (!sample_frame.empty()) {
                const auto img = load_gallery_image(sample_frame);
                if (img) {
                    entry.width = img->width;
                    entry.height = img->height;
                }
            }
            decks.push_back(std::move(entry));
        }
    }

    std::sort(decks.begin(), decks.end(), [](const auto &a, const auto &b) {
        return a.deck < b.deck;
    });
    return decks;
}

std::filesystem::path find_ffmpeg_binary() {
    const char *env_path = std::getenv("FFMPEG_PATH");
    if (env_path && *env_path != '\0') {
        std::error_code ec;
        if (std::filesystem::exists(env_path, ec) && !ec) {
            return std::filesystem::path(env_path);
        }
    }

    const auto exe_dir = runtime_paths().executable_dir;
    if (!exe_dir.empty()) {
#if defined(_WIN32)
        const auto local_ffmpeg = exe_dir / "ffmpeg.exe";
#else
        const auto local_ffmpeg = exe_dir / "ffmpeg";
#endif
        std::error_code ec;
        if (std::filesystem::exists(local_ffmpeg, ec) && !ec) {
            return local_ffmpeg;
        }
    }

    // Default to resolving "ffmpeg" through system PATH
    return "ffmpeg";
}

bool is_ffmpeg_available(std::string *version_out) {
#if defined(__EMSCRIPTEN__)
    return false;
#else
    const auto bin = find_ffmpeg_binary();
    std::string cmd = "\"" + bin.string() + "\" -version 2>&1";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;

    std::array<char, 256> buffer{};
    std::string output;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    const int ret = pclose(pipe);

    if (ret == 0 && output.find("ffmpeg version") != std::string::npos) {
        if (version_out) {
            const auto line_end = output.find_first_of("\r\n");
            *version_out = (line_end != std::string::npos) ? output.substr(0, line_end) : output;
        }
        return true;
    }
    return false;
#endif
}

std::string build_ffmpeg_command(const VideoExportOptions &options, const std::filesystem::path &ffmpeg_bin) {
    const auto bin = ffmpeg_bin.empty() ? find_ffmpeg_binary() : ffmpeg_bin;
    const auto [frame_count, start_num] = inspect_deck_frames(options.deck_dir);
    (void)frame_count;

    auto out_path = options.output_path;
    if (out_path.empty()) {
        std::string deck_name = options.deck_dir.filename().string();
        if (deck_name.empty()) deck_name = "movie";
        const std::string ext = (options.format == VideoFormat::mp4) ? ".mp4" : ".webm";
        const auto downloads = user_downloads_directory();
        if (!downloads.empty()) {
            out_path = downloads / ("noctis_deck_" + deck_name + ext);
        } else {
            out_path = options.deck_dir / ("deck_" + deck_name + ext);
        }
    }

    const double fps = options.fps > 0.0 ? options.fps : 18.2;
    std::ostringstream cmd;
    cmd << "\"" << bin.string() << "\" -y";
    cmd << " -framerate " << fps;
    cmd << " -start_number " << start_num;
    cmd << " -i \"" << (options.deck_dir / "%08d.BMP").string() << "\"";

    if (options.format == VideoFormat::mp4) {
        cmd << " -c:v libx264 -pix_fmt yuv420p -movflags +faststart";
    } else {
        cmd << " -c:v libvpx-vp9 -b:v 0 -crf 30 -pix_fmt yuv420p";
    }

    cmd << " \"" << out_path.string() << "\"";
    return cmd.str();
}

VideoExportStatus export_video_sync(const VideoExportOptions &options) {
    VideoExportStatus status;
    status.in_progress = true;

    const auto [frame_count, start_num] = inspect_deck_frames(options.deck_dir);
    (void)start_num;
    if (frame_count == 0) {
        status.in_progress = false;
        status.finished = true;
        status.success = false;
        status.message = "DECK HAS NO RECORDED FRAMES";
        return status;
    }

    std::string ver;
    if (!is_ffmpeg_available(&ver)) {
        status.in_progress = false;
        status.finished = true;
        status.success = false;
        status.message = "FFMPEG NOT FOUND: INSTALL FFMPEG TO EXPORT DIRECT VIDEOS";
        return status;
    }

    auto out_path = options.output_path;
    if (out_path.empty()) {
        std::string deck_name = options.deck_dir.filename().string();
        if (deck_name.empty()) deck_name = "movie";
        const std::string ext = (options.format == VideoFormat::mp4) ? ".mp4" : ".webm";
        const auto downloads = user_downloads_directory();
        if (!downloads.empty()) {
            out_path = downloads / ("noctis_deck_" + deck_name + ext);
        } else {
            out_path = options.deck_dir / ("deck_" + deck_name + ext);
        }
    }

    std::error_code ec;
    std::filesystem::create_directories(out_path.parent_path(), ec);

    const std::string command = build_ffmpeg_command(options) + " 2>&1";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        status.in_progress = false;
        status.finished = true;
        status.success = false;
        status.message = "FAILED TO SPAWN FFMPEG ENCODER";
        return status;
    }

    std::array<char, 256> buffer{};
    std::string log;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        log += buffer.data();
    }
    const int exit_code = pclose(pipe);

    status.in_progress = false;
    status.finished = true;
    if (exit_code == 0 && std::filesystem::exists(out_path, ec) && std::filesystem::file_size(out_path, ec) > 0) {
        status.success = true;
        status.output_file = out_path;
        status.message = "EXPORTED " + out_path.filename().string();
    } else {
        status.success = false;
        status.message = "FFMPEG ENCODE ERROR (CODE " + std::to_string(exit_code) + ")";
    }

    return status;
}

bool start_video_export_async(const VideoExportOptions &options) {
    std::lock_guard<std::mutex> lock(g_export_mutex);
    if (g_export_status.in_progress) return false;

    g_export_status = VideoExportStatus{};
    g_export_status.in_progress = true;
    g_export_status.message = "ENCODING DECK VIDEO...";

    std::thread worker([opts = options]() {
        const auto result = export_video_sync(opts);
        std::lock_guard<std::mutex> lock(g_export_mutex);
        g_export_status = result;
    });
    worker.detach();
    return true;
}

bool is_video_export_running() {
    std::lock_guard<std::mutex> lock(g_export_mutex);
    return g_export_status.in_progress;
}

VideoExportStatus get_video_export_status() {
    std::lock_guard<std::mutex> lock(g_export_mutex);
    return g_export_status;
}

void reset_video_export_status() {
    std::lock_guard<std::mutex> lock(g_export_mutex);
    g_export_status = VideoExportStatus{};
}

#if defined(__EMSCRIPTEN__)
EM_JS(int, js_media_recorder_supported, (), {
    var canvas = document.getElementById('canvas') || document.querySelector('canvas');
    return (canvas && typeof canvas.captureStream === 'function' && typeof window.MediaRecorder === 'function') ? 1 : 0;
});

EM_JS(int, js_start_canvas_recording, (int fps), {
    try {
        var canvas = document.getElementById('canvas') || document.querySelector('canvas');
        if (!canvas || !window.MediaRecorder) return 0;
        var stream = canvas.captureStream(fps > 0 ? fps : 20);
        var options = { mimeType: 'video/webm;codecs=vp9' };
        if (!MediaRecorder.isTypeSupported(options.mimeType)) {
            options = { mimeType: 'video/webm;codecs=vp8' };
            if (!MediaRecorder.isTypeSupported(options.mimeType)) {
                options = { mimeType: 'video/webm' };
                if (!MediaRecorder.isTypeSupported(options.mimeType)) {
                    options = {};
                }
            }
        }
        window._noctis_recorder_chunks = [];
        window._noctis_recorder = new MediaRecorder(stream, options);
        window._noctis_recorder.ondataavailable = function(e) {
            if (e.data && e.data.size > 0) {
                window._noctis_recorder_chunks.push(e.data);
            }
        };
        window._noctis_recorder.start(100);
        return 1;
    } catch (err) {
        console.error('Noctis IV OM: Failed to start MediaRecorder', err);
        return 0;
    }
});

EM_JS(int, js_stop_canvas_recording, (const char *name_str), {
    try {
        if (!window._noctis_recorder) return 0;
        var name = (name_str ? UTF8ToString(name_str) : 'noctis_movie') + '.webm';
        window._noctis_recorder.onstop = function() {
            var blob = new Blob(window._noctis_recorder_chunks,
                                { type: window._noctis_recorder.mimeType || 'video/webm' });
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.style.display = 'none';
            a.href = url;
            a.download = name;
            document.body.appendChild(a);
            a.click();
            setTimeout(function() {
                document.body.removeChild(a);
                window.URL.revokeObjectURL(url);
            }, 2000);
            window._noctis_recorder = null;
            window._noctis_recorder_chunks = [];
        };
        window._noctis_recorder.stop();
        return 1;
    } catch (err) {
        console.error('Noctis IV OM: Failed to stop MediaRecorder', err);
        return 0;
    }
});

EM_JS(int, js_is_canvas_recording, (), {
    return (window._noctis_recorder && window._noctis_recorder.state === 'recording') ? 1 : 0;
});

EM_JS(int, js_start_deck_export, (const char *deck_dir_str, const char *deck_name_str, double fps), {
    try {
        if (window._noctis_deck_exporting || typeof window.MediaRecorder !== 'function' ||
            typeof window.createImageBitmap !== 'function' ||
            typeof HTMLCanvasElement.prototype.captureStream !== 'function') return 0;

        var deckDir = UTF8ToString(deck_dir_str);
        var deckName = UTF8ToString(deck_name_str);
        var frames = FS.readdir(deckDir)
            .filter(function(name) {
                if (name.length !== 12 || name.slice(8).toLowerCase() !== '.bmp') return false;
                return Array.from(name.slice(0, 8)).every(function(ch) { return ch >= '0' && ch <= '9'; });
            })
            .sort();
        if (!frames.length) return 0;

        var rate = Number.isFinite(fps) && fps > 0 ? fps : 18.2;
        window._noctis_deck_exporting = true;

        (async function() {
            var stream = null;
            try {
                var loadFrame = async function(name) {
                    var bytes = FS.readFile(deckDir + '/' + name);
                    return await createImageBitmap(new Blob([bytes], { type: 'image/bmp' }));
                };

                var first = await loadFrame(frames[0]);
                var canvas = document.createElement('canvas');
                canvas.width = first.width;
                canvas.height = first.height;
                var context = canvas.getContext('2d', { alpha: false });
                context.imageSmoothingEnabled = false;
                context.drawImage(first, 0, 0);
                first.close();

                stream = canvas.captureStream(rate);
                var mimeType = 'video/webm;codecs=vp9';
                if (!MediaRecorder.isTypeSupported(mimeType)) mimeType = 'video/webm;codecs=vp8';
                if (!MediaRecorder.isTypeSupported(mimeType)) mimeType = 'video/webm';
                var options = MediaRecorder.isTypeSupported(mimeType) ? { mimeType: mimeType } : {};
                var chunks = [];
                var recorder = new MediaRecorder(stream, options);
                var stopped = new Promise(function(resolve) { recorder.onstop = resolve; });
                recorder.ondataavailable = function(event) {
                    if (event.data && event.data.size > 0) chunks.push(event.data);
                };
                recorder.start(250);

                var frameDelay = 1000 / rate;
                for (var index = 0; index < frames.length; ++index) {
                    var bitmap = await loadFrame(frames[index]);
                    context.drawImage(bitmap, 0, 0, canvas.width, canvas.height);
                    bitmap.close();
                    await new Promise(function(resolve) { setTimeout(resolve, frameDelay); });
                }

                recorder.stop();
                await stopped;
                var blob = new Blob(chunks, { type: recorder.mimeType || 'video/webm' });
                var url = URL.createObjectURL(blob);
                var anchor = document.createElement('a');
                anchor.style.display = 'none';
                anchor.href = url;
                anchor.download = 'noctis_deck_' + deckName + '.webm';
                document.body.appendChild(anchor);
                anchor.click();
                setTimeout(function() {
                    document.body.removeChild(anchor);
                    URL.revokeObjectURL(url);
                }, 2000);
            } catch (err) {
                console.error('Noctis IV OM: Failed to export movie deck', err);
            } finally {
                if (stream) stream.getTracks().forEach(function(track) { track.stop(); });
                window._noctis_deck_exporting = false;
            }
        })();
        return 1;
    } catch (err) {
        window._noctis_deck_exporting = false;
        console.error('Noctis IV OM: Failed to start movie deck export', err);
        return 0;
    }
});

EM_JS(int, js_is_deck_export_running, (), {
    return window._noctis_deck_exporting ? 1 : 0;
});

bool browser_media_recorder_supported() {
    return js_media_recorder_supported() != 0;
}

bool start_browser_video_recording(int fps) {
    return js_start_canvas_recording(fps) != 0;
}

bool stop_browser_video_recording(const char *deck_name) {
    return js_stop_canvas_recording(deck_name) != 0;
}

bool is_browser_video_recording() {
    return js_is_canvas_recording() != 0;
}

bool start_browser_deck_export(const char *deck_dir, const char *deck_name, double fps) {
    return deck_dir != nullptr && deck_name != nullptr && js_start_deck_export(deck_dir, deck_name, fps) != 0;
}

bool is_browser_deck_export_running() {
    return js_is_deck_export_running() != 0;
}
#endif

} // namespace noctis
