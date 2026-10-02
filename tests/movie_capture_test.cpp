#include "movie_capture.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
int failures = 0;

void expect(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    const std::filesystem::path root = argv[1];
    std::error_code error;
    std::filesystem::remove_all(root, error);

    noctis::MovieRecorder recorder;
    expect(recorder.deck() == 1 && recorder.cadence() == 1, "defaults differ from pinned Moviemaker");
    expect(!recorder.change_deck(-1), "deck moved below one");
    expect(recorder.change_deck(1) && recorder.deck() == 2, "deck increment failed");
    expect(recorder.change_cadence(2) && recorder.cadence() == 3, "cadence adjustment failed");
    recorder.toggle_black_flash();
    expect(recorder.black_flash(), "flash toggle failed");

    noctis::MovieRecorder bounds;
    expect(bounds.change_deck(998) && bounds.deck() == noctis::maximum_movie_deck,
           "maximum deck could not be selected");
    expect(!bounds.change_deck(1), "deck moved above 999");
    expect(bounds.change_cadence(998) && bounds.cadence() == noctis::maximum_movie_cadence,
           "maximum cadence could not be selected");
    expect(!bounds.change_cadence(1), "cadence moved above 999");

    std::filesystem::create_directories(root / "002");
    recorder.toggle_menu();
    expect(recorder.start_or_resume(root) == noctis::MovieStartResult::occupied,
           "occupied deck was not rejected");
    expect(recorder.menu_open() && !recorder.recording(), "occupied deck changed recorder state");
    expect(recorder.change_deck(1), "free deck selection failed");
    expect(recorder.start_or_resume(root) == noctis::MovieStartResult::started, "recording did not start");
    expect(!recorder.change_deck(1) && !recorder.change_cadence(1), "active settings were mutable");

    for (int frame = 0; frame < 8; ++frame) {
        const auto decision = recorder.advance_simulation_frame(false);
        const bool expected_capture = frame == 0 || frame == 3 || frame == 6;
        expect(decision.capture_path.has_value() == expected_capture, "cadence emitted the wrong frame");
        if (decision.capture_path) {
            const auto expected_name = recorder.captured_frames() == 0 ? "00000001.BMP"
                                     : recorder.captured_frames() == 1 ? "00000002.BMP" : "00000003.BMP";
            expect(decision.capture_path->filename() == expected_name, "movie frame name is not sequential");
            std::ofstream(*decision.capture_path).put('x');
            recorder.confirm_capture(true);
        }
    }
    expect(recorder.captured_frames() == 3, "captured frame count is wrong");
    expect(recorder.pause_or_resume() && recorder.paused() && recorder.menu_open(), "pause failed");
    const auto paused = recorder.advance_simulation_frame(false);
    expect(!paused.capture_path && recorder.elapsed_ticks() == 8, "pause advanced recording time");
    expect(recorder.start_or_resume(root) == noctis::MovieStartResult::resumed && recorder.recording(),
           "resume failed");
    expect(recorder.stop() && recorder.deck() == 4, "stop did not advance the deck");

    recorder.toggle_menu();
    expect(recorder.start_or_resume(root) == noctis::MovieStartResult::started, "ascent session did not start");
    for (std::uint16_t frame = 1; frame <= noctis::movie_ascent_cutoff_frames; ++frame) {
        const auto decision = recorder.advance_simulation_frame(true);
        if (decision.capture_path) recorder.confirm_capture(true);
        expect(decision.stopped == (frame == noctis::movie_ascent_cutoff_frames),
               "ascent cutoff happened on the wrong frame");
    }
    expect(!recorder.session_active() && recorder.deck() == 5, "ascent cutoff did not close the deck");

    recorder.toggle_menu();
    expect(recorder.start_or_resume(root) == noctis::MovieStartResult::started,
           "capture-failure session did not start");
    const auto failed_capture = recorder.advance_simulation_frame(false);
    expect(failed_capture.capture_path.has_value(), "capture-failure session emitted no first frame");
    recorder.confirm_capture(false);
    expect(!recorder.session_active() && recorder.deck() == 6,
           "capture failure did not safely stop and advance the deck");

    recorder.reset();
    expect(recorder.deck() == 1 && !recorder.black_flash() && !recorder.session_active(), "reset failed");
    std::filesystem::remove_all(root, error);
    return failures == 0 ? 0 : 1;
}
