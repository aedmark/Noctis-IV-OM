#include "input_recording.h"

#include "atomic_file.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <system_error>
#include <type_traits>
#include <utility>

namespace noctis {
namespace {

constexpr std::uint8_t magic[]              = {'N', 'I', 'R', 'P'};
constexpr std::size_t header_size           = 12;
constexpr std::size_t fixed_frame_size      = 28;
constexpr std::uint32_t maximum_frame_count = 1'000'000;
constexpr std::uint16_t maximum_text_count  = 64;
constexpr std::uintmax_t maximum_recording_bytes =
    header_size +
    static_cast<std::uintmax_t>(maximum_frame_count) * (fixed_frame_size + maximum_text_count * sizeof(std::int32_t));
constexpr std::uint64_t known_flags = (UINT64_C(1) << 40) - 1;

template <typename Value> void append_little_endian(std::vector<std::uint8_t> &bytes, Value value) {
    using Unsigned = std::make_unsigned_t<Value>;
    auto bits      = static_cast<Unsigned>(value);
    for (std::size_t index = 0; index < sizeof(Value); ++index) {
        bytes.push_back(static_cast<std::uint8_t>(bits >> (index * 8)));
    }
}

template <typename Value>
bool read_little_endian(std::span<const std::uint8_t> bytes, std::size_t &offset, Value &value) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(Value)) {
        return false;
    }
    using Unsigned = std::make_unsigned_t<Value>;
    Unsigned bits  = 0;
    for (std::size_t index = 0; index < sizeof(Value); ++index) {
        bits |= static_cast<Unsigned>(bytes[offset++]) << (index * 8);
    }
    value = static_cast<Value>(bits);
    return true;
}

std::uint64_t frame_flags(const InputFrame &frame) {
    std::uint64_t flags = 0;
    const auto set      = [&flags](unsigned bit, bool value) {
        if (value) {
            flags |= UINT64_C(1) << bit;
        }
    };
    set(0, frame.mouse_locked);
    set(1, frame.move_forward);
    set(2, frame.move_backward);
    set(3, frame.move_left);
    set(4, frame.move_right);
    set(5, frame.escape_down);
    set(6, frame.cancel_pressed);
    set(7, frame.mouse_left_down);
    set(8, frame.mouse_right_down);
    set(9, frame.arrow_up_pressed);
    set(10, frame.arrow_down_pressed);
    set(11, frame.arrow_left_pressed);
    set(12, frame.arrow_right_pressed);
    set(13, frame.backspace_pressed);
    set(14, frame.enter_pressed);
    set(15, frame.tab_pressed);
    set(16, frame.apostrophe_pressed);
    set(17, frame.space_pressed);
    set(18, frame.space_down);
    set(19, frame.delete_pressed);
    set(20, frame.minus_pressed);
    set(21, frame.comma_pressed);
    set(22, frame.slash_pressed);
    set(23, frame.semicolon_pressed);
    set(24, frame.plus_pressed);
    set(25, frame.control_down);
    set(26, frame.f1_pressed);
    set(27, frame.f2_pressed);
    set(28, frame.f3_pressed);
    set(29, frame.f4_pressed);
    set(30, frame.page_up_pressed);
    set(31, frame.page_down_pressed);
    set(32, frame.home_pressed);
    set(33, frame.end_pressed);
    set(34, frame.toggle_cursor_pressed);
    set(35, frame.toggle_audio_pressed);
    set(36, frame.toggle_fullscreen_pressed);
    set(37, frame.toggle_aspect_pressed);
    set(38, frame.toggle_upscale_pressed);
    set(39, frame.toggle_crt_pressed);
    return flags;
}

InputFrame frame_from_flags(std::uint64_t flags) {
    InputFrame frame;
    const auto get                  = [flags](unsigned bit) { return (flags & (UINT64_C(1) << bit)) != 0; };
    frame.mouse_locked              = get(0);
    frame.move_forward              = get(1);
    frame.move_backward             = get(2);
    frame.move_left                 = get(3);
    frame.move_right                = get(4);
    frame.escape_down               = get(5);
    frame.cancel_pressed            = get(6);
    frame.mouse_left_down           = get(7);
    frame.mouse_right_down          = get(8);
    frame.arrow_up_pressed          = get(9);
    frame.arrow_down_pressed        = get(10);
    frame.arrow_left_pressed        = get(11);
    frame.arrow_right_pressed       = get(12);
    frame.backspace_pressed         = get(13);
    frame.enter_pressed             = get(14);
    frame.tab_pressed               = get(15);
    frame.apostrophe_pressed        = get(16);
    frame.space_pressed             = get(17);
    frame.space_down                = get(18);
    frame.delete_pressed            = get(19);
    frame.minus_pressed             = get(20);
    frame.comma_pressed             = get(21);
    frame.slash_pressed             = get(22);
    frame.semicolon_pressed         = get(23);
    frame.plus_pressed              = get(24);
    frame.control_down              = get(25);
    frame.f1_pressed                = get(26);
    frame.f2_pressed                = get(27);
    frame.f3_pressed                = get(28);
    frame.f4_pressed                = get(29);
    frame.page_up_pressed           = get(30);
    frame.page_down_pressed         = get(31);
    frame.home_pressed              = get(32);
    frame.end_pressed               = get(33);
    frame.toggle_cursor_pressed     = get(34);
    frame.toggle_audio_pressed      = get(35);
    frame.toggle_fullscreen_pressed = get(36);
    frame.toggle_aspect_pressed     = get(37);
    frame.toggle_upscale_pressed    = get(38);
    frame.toggle_crt_pressed        = get(39);
    return frame;
}

InputRecordingResult failure(InputRecordingStatus status, std::string message) { return {status, std::move(message)}; }

} // namespace

bool append_input_frame(InputRecording &recording, std::uint64_t tick, const InputFrame &frame) {
    if (recording.frames.size() >= maximum_frame_count || frame.text.size() > maximum_text_count ||
        !std::isfinite(frame.mouse_delta_x) || !std::isfinite(frame.mouse_delta_y) ||
        (!recording.frames.empty() && tick <= recording.frames.back().tick)) {
        return false;
    }
    recording.frames.push_back({tick, frame});
    return true;
}

std::vector<std::uint8_t> encode_input_recording(const InputRecording &recording) {
    if (recording.frames.size() > maximum_frame_count ||
        recording.frames.size() > std::numeric_limits<std::uint32_t>::max()) {
        return {};
    }
    std::vector<std::uint8_t> bytes;
    bytes.reserve(header_size + recording.frames.size() * fixed_frame_size);
    bytes.insert(bytes.end(), std::begin(magic), std::end(magic));
    append_little_endian(bytes, input_recording_version);
    append_little_endian<std::uint16_t>(bytes, 0);
    append_little_endian(bytes, static_cast<std::uint32_t>(recording.frames.size()));

    std::uint64_t previous_tick = 0;
    bool has_previous_tick      = false;
    for (const auto &record : recording.frames) {
        if ((has_previous_tick && record.tick <= previous_tick) || record.frame.text.size() > maximum_text_count ||
            !std::isfinite(record.frame.mouse_delta_x) || !std::isfinite(record.frame.mouse_delta_y)) {
            return {};
        }
        append_little_endian(bytes, record.tick);
        append_little_endian(bytes, frame_flags(record.frame));
        append_little_endian(bytes, std::bit_cast<std::uint32_t>(record.frame.mouse_delta_x));
        append_little_endian(bytes, std::bit_cast<std::uint32_t>(record.frame.mouse_delta_y));
        append_little_endian(bytes, static_cast<std::uint16_t>(record.frame.text.size()));
        append_little_endian<std::uint16_t>(bytes, 0);
        for (const auto codepoint : record.frame.text) {
            append_little_endian(bytes, codepoint);
        }
        previous_tick     = record.tick;
        has_previous_tick = true;
    }
    return bytes;
}

InputRecordingResult decode_input_recording(std::span<const std::uint8_t> bytes, InputRecording &recording) {
    if (bytes.size() < header_size || !std::equal(std::begin(magic), std::end(magic), bytes.begin())) {
        return failure(InputRecordingStatus::invalid, "invalid input-recording header");
    }
    std::size_t offset        = sizeof(magic);
    std::uint16_t version     = 0;
    std::uint16_t reserved    = 0;
    std::uint32_t frame_count = 0;
    if (!read_little_endian(bytes, offset, version) || !read_little_endian(bytes, offset, reserved) ||
        !read_little_endian(bytes, offset, frame_count)) {
        return failure(InputRecordingStatus::invalid, "truncated input-recording header");
    }
    if (version != input_recording_version) {
        return failure(InputRecordingStatus::unsupported_version, "unsupported input-recording version");
    }
    if (reserved != 0 || frame_count > maximum_frame_count ||
        frame_count > (bytes.size() - offset) / fixed_frame_size) {
        return failure(InputRecordingStatus::invalid, "invalid input-recording metadata");
    }

    InputRecording decoded;
    decoded.frames.reserve(frame_count);
    for (std::uint32_t index = 0; index < frame_count; ++index) {
        std::uint64_t tick           = 0;
        std::uint64_t flags          = 0;
        std::uint32_t delta_x        = 0;
        std::uint32_t delta_y        = 0;
        std::uint16_t text_count     = 0;
        std::uint16_t frame_reserved = 0;
        if (!read_little_endian(bytes, offset, tick) || !read_little_endian(bytes, offset, flags) ||
            !read_little_endian(bytes, offset, delta_x) || !read_little_endian(bytes, offset, delta_y) ||
            !read_little_endian(bytes, offset, text_count) || !read_little_endian(bytes, offset, frame_reserved)) {
            return failure(InputRecordingStatus::invalid, "truncated input-recording frame");
        }
        if ((flags & ~known_flags) != 0 || frame_reserved != 0 || text_count > maximum_text_count ||
            (!decoded.frames.empty() && tick <= decoded.frames.back().tick)) {
            return failure(InputRecordingStatus::invalid, "invalid input-recording frame");
        }
        auto frame          = frame_from_flags(flags);
        frame.mouse_delta_x = std::bit_cast<float>(delta_x);
        frame.mouse_delta_y = std::bit_cast<float>(delta_y);
        if (!std::isfinite(frame.mouse_delta_x) || !std::isfinite(frame.mouse_delta_y)) {
            return failure(InputRecordingStatus::invalid, "non-finite mouse delta in input recording");
        }
        frame.text.reserve(text_count);
        for (std::uint16_t text_index = 0; text_index < text_count; ++text_index) {
            std::int32_t codepoint = 0;
            if (!read_little_endian(bytes, offset, codepoint)) {
                return failure(InputRecordingStatus::invalid, "truncated input-recording text");
            }
            frame.text.push_back(codepoint);
        }
        decoded.frames.push_back({tick, std::move(frame)});
    }
    if (offset != bytes.size()) {
        return failure(InputRecordingStatus::invalid, "trailing bytes in input recording");
    }
    recording = std::move(decoded);
    return {};
}

InputRecordingResult save_input_recording(const std::filesystem::path &path, const InputRecording &recording) {
    const auto bytes = encode_input_recording(recording);
    if (bytes.empty()) {
        return failure(InputRecordingStatus::invalid, "input recording is not encodable");
    }
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output ||
            !output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size())) ||
            !output.flush()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return failure(InputRecordingStatus::io_error, "could not write input recording");
        }
    }
    if (!atomic_replace(temporary, path)) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return failure(InputRecordingStatus::io_error, "could not publish input recording");
    }
    return {};
}

InputRecordingResult load_input_recording(const std::filesystem::path &path, InputRecording &recording) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);
        return failure(exists || error ? InputRecordingStatus::io_error : InputRecordingStatus::not_found,
                       "could not open input recording");
    }
    const auto end = input.tellg();
    if (end < 0) {
        return failure(InputRecordingStatus::io_error, "could not size input recording");
    }
    const auto length = static_cast<std::uintmax_t>(static_cast<std::streamoff>(end));
    if (length > maximum_recording_bytes) {
        return failure(InputRecordingStatus::invalid, "input recording is too large");
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!bytes.empty() &&
        !input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
        return failure(InputRecordingStatus::io_error, "could not read input recording");
    }
    return decode_input_recording(bytes, recording);
}

InputReplay::InputReplay(InputRecording recording) : recording_(std::move(recording)) {}

InputFrame InputReplay::frame_for_tick(std::uint64_t tick) {
    if (has_last_tick_ && tick <= last_tick_) {
        missed_input_ = true;
        return {};
    }
    last_tick_     = tick;
    has_last_tick_ = true;
    while (next_frame_ < recording_.frames.size() && recording_.frames[next_frame_].tick < tick) {
        missed_input_ = true;
        ++next_frame_;
    }
    if (next_frame_ < recording_.frames.size() && recording_.frames[next_frame_].tick == tick) {
        return recording_.frames[next_frame_++].frame;
    }
    return {};
}

void InputReplay::reset() {
    next_frame_    = 0;
    last_tick_     = 0;
    has_last_tick_ = false;
    missed_input_  = false;
}

} // namespace noctis
