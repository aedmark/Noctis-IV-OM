#include "display.h"

#include <cmath>
#include <cstdio>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "display test failure: %s\n", message);
        return false;
    }
    return true;
}
} // namespace

int main() {
    bool ok = true;

    // 1. Invalid dimensions
    {
        const auto vp = noctis::calculate_viewport(0, 0, noctis::AspectRatioMode::crt_4_3);
        ok &= require(vp.width == 0.0f && vp.height == 0.0f, "zero size should yield zero viewport");
    }

    // 2. 1280x720 window (16:9 widescreen) in 4:3 CRT mode -> Pillarboxed
    {
        const auto vp = noctis::calculate_viewport(1280, 720, noctis::AspectRatioMode::crt_4_3);
        // Height should be 720, Width should be 720 * (4/3) = 960
        // Pillarbox X offset should be (1280 - 960) / 2 = 160
        ok &= require(vp.height == 720.0f, "4:3 in 1280x720: height should be 720");
        ok &= require(vp.width == 960.0f, "4:3 in 1280x720: width should be 960");
        ok &= require(vp.x == 160.0f, "4:3 in 1280x720: x offset should be 160");
        ok &= require(vp.y == 0.0f, "4:3 in 1280x720: y offset should be 0");
        ok &= require(std::abs(vp.width / vp.height - 4.0f / 3.0f) < 0.01f, "4:3 ratio verified");
    }

    // 3. 1920x1080 window (1080p 16:9) in 4:3 CRT mode -> Pillarboxed
    {
        const auto vp = noctis::calculate_viewport(1920, 1080, noctis::AspectRatioMode::crt_4_3);
        // Height = 1080, Width = 1080 * 4/3 = 1440, X = (1920 - 1440) / 2 = 240
        ok &= require(vp.height == 1080.0f, "4:3 in 1080p: height should be 1080");
        ok &= require(vp.width == 1440.0f, "4:3 in 1080p: width should be 1440");
        ok &= require(vp.x == 240.0f, "4:3 in 1080p: x offset should be 240");
        ok &= require(vp.y == 0.0f, "4:3 in 1080p: y offset should be 0");
    }

    // 4. Tall window (e.g. 800x800 square 1:1) in 4:3 CRT mode -> Letterboxed
    {
        const auto vp = noctis::calculate_viewport(800, 800, noctis::AspectRatioMode::crt_4_3);
        // Width = 800, Height = 800 / (4/3) = 600, Y = (800 - 600) / 2 = 100
        ok &= require(vp.width == 800.0f, "4:3 in 800x800: width should be 800");
        ok &= require(vp.height == 600.0f, "4:3 in 800x800: height should be 600");
        ok &= require(vp.x == 0.0f, "4:3 in 800x800: x offset should be 0");
        ok &= require(vp.y == 100.0f, "4:3 in 800x800: y offset should be 100");
    }

    // 5. 16:10 Pixel-Exact mode in 1280x720 window
    {
        const auto vp = noctis::calculate_viewport(1280, 720, noctis::AspectRatioMode::pixel_16_10);
        // Height = 720, Width = 720 * 1.6 = 1152, X = (1280 - 1152) / 2 = 64
        ok &= require(vp.height == 720.0f, "16:10 in 1280x720: height should be 720");
        ok &= require(vp.width == 1152.0f, "16:10 in 1280x720: width should be 1152");
        ok &= require(vp.x == 64.0f, "16:10 in 1280x720: x offset should be 64");
        ok &= require(vp.y == 0.0f, "16:10 in 1280x720: y offset should be 0");
    }

    // 6. 16:9 Stretch mode in 1280x720 window
    {
        const auto vp = noctis::calculate_viewport(1280, 720, noctis::AspectRatioMode::stretch_16_9);
        ok &= require(vp.width == 1280.0f && vp.height == 720.0f, "16:9 stretch width/height");
        ok &= require(vp.x == 0.0f && vp.y == 0.0f, "16:9 stretch x/y");
    }

    // 7. Cycle aspect ratio modes
    {
        auto mode = noctis::AspectRatioMode::crt_4_3;
        mode      = noctis::cycle_aspect_ratio_mode(mode);
        ok &= require(mode == noctis::AspectRatioMode::pixel_16_10, "cycle to 16:10");
        mode = noctis::cycle_aspect_ratio_mode(mode);
        ok &= require(mode == noctis::AspectRatioMode::stretch_16_9, "cycle to 16:9");
        mode = noctis::cycle_aspect_ratio_mode(mode);
        ok &= require(mode == noctis::AspectRatioMode::crt_4_3, "cycle back to 4:3");
    }

    // 8. Aspect mode names
    {
        ok &= require(std::string(noctis::aspect_ratio_mode_name(noctis::AspectRatioMode::crt_4_3)) ==
                          "ASPECT: 4:3 CRT",
                      "mode name 4:3");
        ok &= require(std::string(noctis::aspect_ratio_mode_name(noctis::AspectRatioMode::pixel_16_10)) ==
                          "ASPECT: 16:10 SQUARE",
                      "mode name 16:10");
        ok &= require(std::string(noctis::aspect_ratio_mode_name(noctis::AspectRatioMode::stretch_16_9)) ==
                          "ASPECT: 16:9 STRETCH",
                      "mode name 16:9");
    }

    // 9. Timewarp slider touch
    {
        noctis::touch_timewarp_slider();
        ok &= require(true, "touch_timewarp_slider did not crash");
    }

    return ok ? 0 : 1;
}
