#include "upscale.h"

#include <algorithm>

namespace noctis {

namespace {
UpscaleMode g_upscale_mode = UpscaleMode::crisp_pixel;
} // namespace

UpscaleMode cycle_upscale_mode(UpscaleMode current) {
    switch (current) {
        case UpscaleMode::crisp_pixel:     return UpscaleMode::edge_scale2x;
        case UpscaleMode::edge_scale2x:    return UpscaleMode::smooth_bilinear;
        case UpscaleMode::smooth_bilinear: return UpscaleMode::crisp_pixel;
    }
    return UpscaleMode::crisp_pixel;
}

const char *upscale_mode_name(UpscaleMode mode) {
    switch (mode) {
        case UpscaleMode::crisp_pixel:     return "UPSCALE: CRISP PIXEL (1X)";
        case UpscaleMode::edge_scale2x:    return "UPSCALE: SCALE2X (EDGE)";
        case UpscaleMode::smooth_bilinear: return "UPSCALE: SMOOTH (BILINEAR)";
    }
    return "UPSCALE: CRISP PIXEL (1X)";
}

UpscaleMode get_upscale_mode() {
    return g_upscale_mode;
}

void set_upscale_mode(UpscaleMode mode) {
    g_upscale_mode = mode;
}

void scale2x_rgba(const std::uint32_t *src, int width, int height, std::uint32_t *dst) {
    if (!src || !dst || width <= 0 || height <= 0) return;

    const int dst_width = width * 2;

    for (int y = 0; y < height; ++y) {
        const int y_prev = (y > 0) ? (y - 1) : 0;
        const int y_next = (y < height - 1) ? (y + 1) : (height - 1);

        const std::uint32_t *row_curr  = src + y * width;
        const std::uint32_t *row_above = src + y_prev * width;
        const std::uint32_t *row_below = src + y_next * width;

        std::uint32_t *dst_row0 = dst + (y * 2) * dst_width;
        std::uint32_t *dst_row1 = dst + (y * 2 + 1) * dst_width;

        for (int x = 0; x < width; ++x) {
            const int x_prev = (x > 0) ? (x - 1) : 0;
            const int x_next = (x < width - 1) ? (x + 1) : (width - 1);

            const std::uint32_t p = row_curr[x];
            const std::uint32_t a = row_above[x];
            const std::uint32_t b = row_curr[x_next];
            const std::uint32_t c = row_curr[x_prev];
            const std::uint32_t d = row_below[x];

            // Scale2x / AdvMame2x rules:
            // Top-left: E0
            const std::uint32_t e0 = (c == a && c != d && a != b) ? a : p;
            // Top-right: E1
            const std::uint32_t e1 = (a == b && a != c && b != d) ? b : p;
            // Bottom-left: E2
            const std::uint32_t e2 = (d == c && d != b && c != a) ? c : p;
            // Bottom-right: E3
            const std::uint32_t e3 = (b == d && b != a && d != c) ? d : p;

            const int dst_x = x * 2;
            dst_row0[dst_x + 0] = e0;
            dst_row0[dst_x + 1] = e1;
            dst_row1[dst_x + 0] = e2;
            dst_row1[dst_x + 1] = e3;
        }
    }
}

void scale2x_indexed(const std::uint8_t *src, int width, int height, std::uint8_t *dst) {
    if (!src || !dst || width <= 0 || height <= 0) return;

    const int dst_width = width * 2;

    for (int y = 0; y < height; ++y) {
        const int y_prev = (y > 0) ? (y - 1) : 0;
        const int y_next = (y < height - 1) ? (y + 1) : (height - 1);

        const std::uint8_t *row_curr  = src + y * width;
        const std::uint8_t *row_above = src + y_prev * width;
        const std::uint8_t *row_below = src + y_next * width;

        std::uint8_t *dst_row0 = dst + (y * 2) * dst_width;
        std::uint8_t *dst_row1 = dst + (y * 2 + 1) * dst_width;

        for (int x = 0; x < width; ++x) {
            const int x_prev = (x > 0) ? (x - 1) : 0;
            const int x_next = (x < width - 1) ? (x + 1) : (width - 1);

            const std::uint8_t p = row_curr[x];
            const std::uint8_t a = row_above[x];
            const std::uint8_t b = row_curr[x_next];
            const std::uint8_t c = row_curr[x_prev];
            const std::uint8_t d = row_below[x];

            const std::uint8_t e0 = (c == a && c != d && a != b) ? a : p;
            const std::uint8_t e1 = (a == b && a != c && b != d) ? b : p;
            const std::uint8_t e2 = (d == c && d != b && c != a) ? c : p;
            const std::uint8_t e3 = (b == d && b != a && d != c) ? d : p;

            const int dst_x = x * 2;
            dst_row0[dst_x + 0] = e0;
            dst_row0[dst_x + 1] = e1;
            dst_row1[dst_x + 0] = e2;
            dst_row1[dst_x + 1] = e3;
        }
    }
}

} // namespace noctis
