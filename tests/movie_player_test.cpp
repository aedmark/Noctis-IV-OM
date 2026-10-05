#include "movie_player.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "movie_player_test: %s\n", message);
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

namespace noctis {
bool is_cursor_lock_wanted() {
    return false;
}
void play_goesnet_chime(bool) {}
void play_cockpit_button() {}
} // namespace noctis

int main(int argc, char **argv) {
    using namespace noctis;
    if (argc != 2) {
        std::fputs("usage: movie_player_test SCRATCH_DIR\n", stderr);
        return 2;
    }

    const std::filesystem::path root(argv[1]);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / "movies" / "001", ec);
    std::filesystem::create_directories(root / "movies" / "002", ec);

    for (int i = 1; i <= 6; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "%08d.BMP", i);
        write_dummy_bmp(root / "movies" / "001" / name);
    }

    bool ok = true;

    // 1. Opening invalid / non-existent deck
    ok &= require(!open_movie_player(root / "movies", 999), "opening non-existent deck must return false");
    ok &= require(!open_movie_player(root / "nonexistent", 1), "opening non-existent directory must return false");
    ok &= require(!movie_player_open(), "player must remain closed after failed open");

    // 2. Open valid deck 001
    ok &= require(open_movie_player(root / "movies", 1), "opening valid deck 1 must succeed");
    ok &= require(movie_player_open(), "movie_player_open() must be true");
    ok &= require(movie_player_current_deck() == 1, "current deck must be 1");
    ok &= require(movie_player_total_frames() == 6, "total frames must be 6");
    ok &= require(movie_player_current_frame() == 0, "initial frame must be 0");
    ok &= require(movie_player_is_playing(), "initial state should be playing");
    ok &= require(movie_player_is_looping(), "initial loop mode should be on");

    // 3. Step forward
    InputFrame frame_step_right;
    frame_step_right.arrow_right_pressed = true;
    ok &= require(movie_player_input(frame_step_right), "step right should consume input");
    ok &= require(movie_player_current_frame() == 1, "frame after step right should be 1");
    ok &= require(!movie_player_is_playing(), "stepping should pause playback");

    // 4. Step backward
    InputFrame frame_step_left;
    frame_step_left.arrow_left_pressed = true;
    ok &= require(movie_player_input(frame_step_left), "step left should consume input");
    ok &= require(movie_player_current_frame() == 0, "frame after step left should be 0");

    // 5. Jump to end and home
    InputFrame frame_end;
    frame_end.end_pressed = true;
    ok &= require(movie_player_input(frame_end), "end key should consume input");
    ok &= require(movie_player_current_frame() == 5, "frame after end key should be 5");

    InputFrame frame_home;
    frame_home.home_pressed = true;
    ok &= require(movie_player_input(frame_home), "home key should consume input");
    ok &= require(movie_player_current_frame() == 0, "frame after home key should be 0");

    // 6. Play / Pause toggle
    InputFrame frame_space;
    frame_space.space_pressed = true;
    ok &= require(movie_player_input(frame_space), "space should consume input");
    ok &= require(movie_player_is_playing(), "space should resume playing");

    frame_space = InputFrame{};
    frame_space.space_pressed = true;
    ok &= require(movie_player_input(frame_space), "second space should consume input");
    ok &= require(!movie_player_is_playing(), "second space should pause playing");

    // 7. Loop toggle
    InputFrame frame_loop;
    frame_loop.text.push_back('l');
    ok &= require(movie_player_input(frame_loop), "'l' key should consume input");
    ok &= require(!movie_player_is_looping(), "loop mode should be disabled");

    // 8. Close with escape
    InputFrame frame_esc;
    frame_esc.escape_down = true;
    ok &= require(movie_player_input(frame_esc), "escape should consume input");
    ok &= require(!movie_player_open(), "player should be closed after escape");

    shutdown_movie_player();
    std::filesystem::remove_all(root, ec);
    return ok ? 0 : 1;
}
