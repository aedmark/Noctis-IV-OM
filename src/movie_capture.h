#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace noctis {

inline constexpr std::uint16_t minimum_movie_deck = 1;
inline constexpr std::uint16_t maximum_movie_deck = 999;
inline constexpr std::uint16_t minimum_movie_cadence = 1;
inline constexpr std::uint16_t maximum_movie_cadence = 999;
inline constexpr std::uint16_t movie_ascent_cutoff_frames = 100;

enum class MovieStartResult : std::uint8_t { started, resumed, occupied, io_error, unavailable };

struct MovieFrameDecision {
    std::optional<std::filesystem::path> capture_path;
    bool stopped{};
};

class MovieRecorder {
public:
    void reset();
    void toggle_menu();
    void close_menu();
    bool change_deck(int delta);
    bool change_cadence(int delta);
    void toggle_black_flash();

    MovieStartResult start_or_resume(const std::filesystem::path &root);
    bool stop();
    bool pause_or_resume();
    MovieFrameDecision advance_simulation_frame(bool ascending_from_surface);
    void confirm_capture(bool succeeded);

    [[nodiscard]] bool menu_open() const { return menu_open_; }
    [[nodiscard]] bool recording() const { return recording_; }
    [[nodiscard]] bool paused() const { return paused_; }
    [[nodiscard]] bool session_active() const { return recording_ || paused_; }
    [[nodiscard]] bool black_flash() const { return black_flash_; }
    [[nodiscard]] std::uint16_t deck() const { return deck_; }
    [[nodiscard]] std::uint16_t cadence() const { return cadence_; }
    [[nodiscard]] std::uint32_t captured_frames() const { return captured_frames_; }
    [[nodiscard]] std::uint64_t elapsed_ticks() const { return elapsed_ticks_; }
    [[nodiscard]] double captured_fps() const;
    [[nodiscard]] std::filesystem::path deck_path(const std::filesystem::path &root) const;
    [[nodiscard]] bool deck_occupied(const std::filesystem::path &root) const;

private:
    void advance_deck();

    bool menu_open_{};
    bool recording_{};
    bool paused_{};
    bool black_flash_{};
    std::uint16_t deck_{minimum_movie_deck};
    std::uint16_t cadence_{minimum_movie_cadence};
    std::uint16_t frames_until_capture_{};
    std::uint16_t ascent_frames_{};
    std::uint32_t next_frame_{1};
    std::uint32_t captured_frames_{};
    std::uint64_t elapsed_ticks_{};
    std::filesystem::path active_root_;
};

} // namespace noctis
