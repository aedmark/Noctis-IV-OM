#include "video_export.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "video_export_test: %s\n", message);
    }
    return condition;
}

void write_dummy_bmp(const std::filesystem::path &path) {
    std::ofstream out(path, std::ios::binary);
    const std::uint32_t width = 320;
    const std::uint32_t height = 200;
    const std::uint32_t stride = (width + 3) & ~3U;
    std::vector<std::uint8_t> dummy(1078 + stride * height, 0);
    dummy[0] = 'B'; dummy[1] = 'M';
    const auto file_size = static_cast<std::uint32_t>(dummy.size());
    dummy[2] = static_cast<std::uint8_t>(file_size & 0xFF);
    dummy[3] = static_cast<std::uint8_t>((file_size >> 8) & 0xFF);
    dummy[4] = static_cast<std::uint8_t>((file_size >> 16) & 0xFF);
    dummy[5] = static_cast<std::uint8_t>((file_size >> 24) & 0xFF);
    dummy[10] = 0x36; dummy[11] = 0x04; // 1078 offset
    dummy[14] = 40; // biSize
    dummy[18] = 0x40; dummy[19] = 0x01; // 320
    dummy[22] = static_cast<std::uint8_t>(200); // 200
    dummy[26] = 1; // biPlanes
    dummy[28] = 8; // biBitCount
    dummy[47] = 1; // 256 colors used
    for (std::size_t c = 0; c < 256; ++c) {
        dummy[54 + c * 4 + 0] = static_cast<std::uint8_t>(c);
        dummy[54 + c * 4 + 1] = static_cast<std::uint8_t>(255 - c);
        dummy[54 + c * 4 + 2] = static_cast<std::uint8_t>((c * 2) % 256);
    }
    out.write(reinterpret_cast<const char *>(dummy.data()), dummy.size());
}

} // namespace

int main(int argc, char **argv) {
    using namespace noctis;
    if (argc != 2) {
        std::fputs("usage: video_export_test SCRATCH_DIR\n", stderr);
        return 2;
    }

    const std::filesystem::path root(argv[1]);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / "movies" / "001", ec);
    std::filesystem::create_directories(root / "movies" / "002", ec);

    bool ok = true;

    // 1. Deck scanning on mock directories
    for (int i = 1; i <= 5; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "%08d.BMP", i);
        write_dummy_bmp(root / "movies" / "001" / name);
    }
    for (int i = 1; i <= 3; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "%08d.BMP", i);
        write_dummy_bmp(root / "movies" / "002" / name);
    }

    const auto decks = scan_movie_decks(root / "movies");
    ok &= require(decks.size() == 2, "must discover exactly 2 decks");
    if (decks.size() == 2) {
        ok &= require(decks[0].deck == 1 && decks[0].frame_count == 5, "deck 1 properties mismatch");
        ok &= require(decks[1].deck == 2 && decks[1].frame_count == 3, "deck 2 properties mismatch");
        ok &= require(decks[0].deck_str == "001", "deck 1 name string mismatch");
        ok &= require(decks[1].deck_str == "002", "deck 2 name string mismatch");
    }

    // 2. Command generation
    VideoExportOptions opts;
    opts.deck_dir = root / "movies" / "001";
    opts.fps = 18.2;
    opts.format = VideoFormat::mp4;
    opts.output_path = root / "output.mp4";

    const std::string cmd_mp4 = build_ffmpeg_command(opts, "ffmpeg");
    ok &= require(cmd_mp4.find("-framerate 18.2") != std::string::npos, "framerate in cmd mismatch");
    ok &= require(cmd_mp4.find("-c:v libx264") != std::string::npos, "H.264 codec in cmd mismatch");
    ok &= require(cmd_mp4.find("-pix_fmt yuv420p") != std::string::npos, "yuv420p pix_fmt in cmd mismatch");
    ok &= require(cmd_mp4.find("output.mp4") != std::string::npos, "output path in cmd mismatch");

    opts.format = VideoFormat::webm;
    opts.output_path = root / "output.webm";
    const std::string cmd_webm = build_ffmpeg_command(opts, "ffmpeg");
    ok &= require(cmd_webm.find("-c:v libvpx-vp9") != std::string::npos, "VP9 codec in cmd mismatch");
    ok &= require(cmd_webm.find("output.webm") != std::string::npos, "output webm in cmd mismatch");

    // 3. Empty deck handling
    VideoExportOptions empty_opts;
    empty_opts.deck_dir = root / "movies" / "999";
    const auto empty_res = export_video_sync(empty_opts);
    ok &= require(!empty_res.success, "empty deck should fail export");
    ok &= require(empty_res.message.find("NO RECORDED FRAMES") != std::string::npos, "empty deck message mismatch");

    // 4. FFmpeg availability check
    std::string version;
    const bool ffmpeg_found = is_ffmpeg_available(&version);
    if (ffmpeg_found) {
        ok &= require(!version.empty(), "version string should not be empty when ffmpeg is available");
        // Test actual export if ffmpeg is present on this machine!
        opts.format = VideoFormat::mp4;
        opts.output_path = root / "actual_test.mp4";
        const auto real_res = export_video_sync(opts);
        ok &= require(real_res.success, "real ffmpeg export failed");
        ok &= require(std::filesystem::exists(opts.output_path), "exported MP4 file does not exist");
        ok &= require(std::filesystem::file_size(opts.output_path) > 0, "exported MP4 file is empty");
    }

    std::filesystem::remove_all(root, ec);
    return ok ? 0 : 1;
}
