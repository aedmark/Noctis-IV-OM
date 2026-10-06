#pragma once

#include "input.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace noctis {

inline constexpr std::uint16_t input_recording_version = 1;

struct RecordedInputFrame {
    std::uint64_t tick = 0;
    InputFrame frame;

    bool operator==(const RecordedInputFrame &) const = default;
};

struct InputRecording {
    std::vector<RecordedInputFrame> frames;

    bool operator==(const InputRecording &) const = default;
};

enum class InputRecordingStatus {
    ok,
    not_found,
    invalid,
    unsupported_version,
    io_error,
};

struct InputRecordingResult {
    InputRecordingStatus status = InputRecordingStatus::ok;
    std::string message;

    [[nodiscard]] explicit operator bool() const { return status == InputRecordingStatus::ok; }
};

[[nodiscard]] bool append_input_frame(InputRecording &recording, std::uint64_t tick, const InputFrame &frame);
[[nodiscard]] std::vector<std::uint8_t> encode_input_recording(const InputRecording &recording);
[[nodiscard]] InputRecordingResult decode_input_recording(std::span<const std::uint8_t> bytes,
                                                          InputRecording &recording);
[[nodiscard]] InputRecordingResult save_input_recording(const std::filesystem::path &path,
                                                        const InputRecording &recording);
[[nodiscard]] InputRecordingResult load_input_recording(const std::filesystem::path &path, InputRecording &recording);

class InputReplay {
  public:
    explicit InputReplay(InputRecording recording);

    [[nodiscard]] InputFrame frame_for_tick(std::uint64_t tick);
    [[nodiscard]] bool complete() const { return next_frame_ == recording_.frames.size(); }
    [[nodiscard]] bool missed_input() const { return missed_input_; }
    void reset();

  private:
    InputRecording recording_;
    std::size_t next_frame_  = 0;
    std::uint64_t last_tick_ = 0;
    bool has_last_tick_      = false;
    bool missed_input_       = false;
};

} // namespace noctis
