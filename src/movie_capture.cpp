#include "movie_capture.h"

#include "simulation_clock.h"

#include <cstdio>

namespace noctis {

void MovieRecorder::reset() { *this = {}; }

void MovieRecorder::toggle_menu() { menu_open_ = !menu_open_; }

void MovieRecorder::close_menu() { menu_open_ = false; }

bool MovieRecorder::change_deck(int delta) {
    if (session_active() || delta == 0) return false;
    const auto changed = static_cast<int>(deck_) + delta;
    if (changed < minimum_movie_deck || changed > maximum_movie_deck) return false;
    deck_ = static_cast<std::uint16_t>(changed);
    return true;
}

bool MovieRecorder::change_cadence(int delta) {
    if (session_active() || delta == 0) return false;
    const auto changed = static_cast<int>(cadence_) + delta;
    if (changed < minimum_movie_cadence || changed > maximum_movie_cadence) return false;
    cadence_ = static_cast<std::uint16_t>(changed);
    return true;
}

void MovieRecorder::toggle_black_flash() {
    if (!session_active()) black_flash_ = !black_flash_;
}

std::filesystem::path MovieRecorder::deck_path(const std::filesystem::path &root) const {
    char name[4];
    std::snprintf(name, sizeof(name), "%03u", deck_);
    return root / name;
}

bool MovieRecorder::deck_occupied(const std::filesystem::path &root) const {
    std::error_code error;
    return std::filesystem::exists(deck_path(root), error) || static_cast<bool>(error);
}

MovieStartResult MovieRecorder::start_or_resume(const std::filesystem::path &root) {
    if (recording_) return MovieStartResult::unavailable;
    if (paused_) {
        paused_ = false;
        recording_ = true;
        menu_open_ = false;
        return MovieStartResult::resumed;
    }
    if (!menu_open_) return MovieStartResult::unavailable;

    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error) return MovieStartResult::io_error;
    const auto directory = deck_path(root);
    if (!std::filesystem::create_directory(directory, error)) {
        return error ? MovieStartResult::io_error : MovieStartResult::occupied;
    }

    active_root_ = root;
    recording_ = true;
    menu_open_ = false;
    frames_until_capture_ = 0;
    ascent_frames_ = 0;
    next_frame_ = 1;
    captured_frames_ = 0;
    elapsed_ticks_ = 0;
    return MovieStartResult::started;
}

void MovieRecorder::advance_deck() {
    if (deck_ < maximum_movie_deck) ++deck_;
}

bool MovieRecorder::stop() {
    if (!session_active()) return false;
    recording_ = false;
    paused_ = false;
    menu_open_ = false;
    ascent_frames_ = 0;
    advance_deck();
    return true;
}

bool MovieRecorder::pause_or_resume() {
    if (recording_) {
        recording_ = false;
        paused_ = true;
        menu_open_ = true;
        return true;
    }
    if (paused_) {
        paused_ = false;
        recording_ = true;
        menu_open_ = false;
        return true;
    }
    return false;
}

MovieFrameDecision MovieRecorder::advance_simulation_frame(bool ascending_from_surface) {
    MovieFrameDecision decision;
    if (!recording_) return decision;

    ++elapsed_ticks_;
    if (frames_until_capture_ == 0) {
        char filename[13];
        std::snprintf(filename, sizeof(filename), "%08u.BMP", next_frame_);
        decision.capture_path = deck_path(active_root_) / filename;
        frames_until_capture_ = static_cast<std::uint16_t>(cadence_ - 1);
    } else {
        --frames_until_capture_;
    }

    if (ascending_from_surface) {
        ++ascent_frames_;
        if (ascent_frames_ >= movie_ascent_cutoff_frames) {
            decision.stopped = stop();
        }
    } else {
        ascent_frames_ = 0;
    }
    return decision;
}

void MovieRecorder::confirm_capture(bool succeeded) {
    if (succeeded) {
        ++captured_frames_;
        ++next_frame_;
    } else {
        stop();
    }
}

double MovieRecorder::captured_fps() const {
    if (elapsed_ticks_ == 0) return 0.0;
    const auto seconds = static_cast<double>(elapsed_ticks_ * simulation_tick_milliseconds) / 1000.0;
    return static_cast<double>(captured_frames_) / seconds;
}

} // namespace noctis
