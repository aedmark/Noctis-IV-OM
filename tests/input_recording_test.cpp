#include "input_recording.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace {

noctis::InputFrame full_frame() {
    noctis::InputFrame frame;
    frame.mouse_locked = false;
    frame.move_forward = frame.move_backward = frame.move_left = frame.move_right = true;
    frame.escape_down = frame.cancel_pressed = true;
    frame.mouse_left_down = frame.mouse_right_down = true;
    frame.mouse_delta_x                            = -12.5F;
    frame.mouse_delta_y                            = 7.25F;
    frame.text                                     = {'N', 0x03A9, 0x1F680};
    frame.arrow_up_pressed = frame.arrow_down_pressed = true;
    frame.arrow_left_pressed = frame.arrow_right_pressed = true;
    frame.backspace_pressed = frame.enter_pressed = frame.tab_pressed = true;
    frame.apostrophe_pressed = frame.space_pressed = frame.space_down = true;
    frame.delete_pressed = frame.minus_pressed = frame.comma_pressed = true;
    frame.slash_pressed = frame.semicolon_pressed = frame.plus_pressed = true;
    frame.control_down                                                 = true;
    frame.f1_pressed = frame.f2_pressed = frame.f3_pressed = frame.f4_pressed = true;
    frame.page_up_pressed = frame.page_down_pressed = frame.home_pressed = frame.end_pressed = true;
    frame.toggle_cursor_pressed = frame.toggle_audio_pressed = true;
    frame.toggle_fullscreen_pressed = frame.toggle_aspect_pressed = true;
    frame.toggle_upscale_pressed = frame.toggle_crt_pressed = true;
    return frame;
}

bool rejects(std::vector<std::uint8_t> bytes, noctis::InputRecordingStatus expected) {
    noctis::InputRecording unchanged;
    unchanged.frames.push_back({99, {}});
    const auto original = unchanged;
    const auto result   = noctis::decode_input_recording(bytes, unchanged);
    return result.status == expected && unchanged == original;
}

} // namespace

int main(int argc, char **argv) {
    bool ok = argc == 2;

    noctis::InputRecording recording;
    const auto complete = full_frame();
    noctis::InputFrame sparse;
    sparse.text = {'r'};
    ok &= noctis::append_input_frame(recording, 0, complete);
    ok &= noctis::append_input_frame(recording, 7, sparse);
    ok &= !noctis::append_input_frame(recording, 7, {});
    noctis::InputFrame invalid;
    invalid.mouse_delta_x = std::numeric_limits<float>::infinity();
    ok &= !noctis::append_input_frame(recording, 8, invalid);

    const auto bytes = noctis::encode_input_recording(recording);
    ok &= bytes.size() == 12 + 28 + complete.text.size() * 4 + 28 + sparse.text.size() * 4;
    ok &= bytes.size() >= 8 && bytes[0] == 'N' && bytes[1] == 'I' && bytes[2] == 'R' && bytes[3] == 'P';
    ok &= bytes[4] == noctis::input_recording_version && bytes[5] == 0;

    noctis::InputRecording decoded;
    const auto decoded_result = noctis::decode_input_recording(bytes, decoded);
    ok &= static_cast<bool>(decoded_result) && decoded == recording;

    auto corrupt = bytes;
    corrupt[0]   = 'X';
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);
    corrupt    = bytes;
    corrupt[4] = static_cast<std::uint8_t>(noctis::input_recording_version + 1);
    ok &= rejects(corrupt, noctis::InputRecordingStatus::unsupported_version);
    corrupt    = bytes;
    corrupt[6] = 1;
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);
    corrupt = bytes;
    corrupt[25] |= 1;
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);
    corrupt     = bytes;
    corrupt[28] = 0;
    corrupt[29] = 0;
    corrupt[30] = 0x80;
    corrupt[31] = 0x7f;
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);
    corrupt = bytes;
    corrupt.pop_back();
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);
    corrupt = bytes;
    corrupt.push_back(0);
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);
    noctis::InputRecording ordered;
    ok &= noctis::append_input_frame(ordered, 1, {});
    ok &= noctis::append_input_frame(ordered, 2, {});
    corrupt     = noctis::encode_input_recording(ordered);
    corrupt[40] = 1;
    corrupt[41] = corrupt[42] = corrupt[43] = corrupt[44] = corrupt[45] = corrupt[46] = corrupt[47] = 0;
    ok &= rejects(corrupt, noctis::InputRecordingStatus::invalid);

    noctis::InputReplay replay(decoded);
    ok &= replay.frame_for_tick(0) == complete;
    for (std::uint64_t tick = 1; tick < 7; ++tick) {
        ok &= replay.frame_for_tick(tick) == noctis::InputFrame{};
    }
    ok &= replay.frame_for_tick(7) == sparse;
    ok &= replay.complete() && !replay.missed_input();
    replay.reset();
    (void) replay.frame_for_tick(1);
    ok &= replay.missed_input() && !replay.complete();

    if (argc == 2) {
        const std::filesystem::path path(argv[1]);
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        const auto saved = noctis::save_input_recording(path, recording);
        noctis::InputRecording loaded;
        const auto loaded_result = noctis::load_input_recording(path, loaded);
        ok &= static_cast<bool>(saved) && static_cast<bool>(loaded_result) && loaded == recording;
        std::filesystem::remove(path, ignored);
    }

    return ok ? 0 : 1;
}
