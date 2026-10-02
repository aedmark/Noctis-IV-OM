#include "upscale.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "upscale test failure: %s\n", message);
        return false;
    }
    return true;
}
} // namespace

int main() {
    bool ok = true;

    // 1. Mode cycling and naming
    {
        auto mode = noctis::UpscaleMode::crisp_pixel;
        mode = noctis::cycle_upscale_mode(mode);
        ok &= require(mode == noctis::UpscaleMode::edge_scale2x, "cycle to edge_scale2x");
        mode = noctis::cycle_upscale_mode(mode);
        ok &= require(mode == noctis::UpscaleMode::smooth_bilinear, "cycle to smooth_bilinear");
        mode = noctis::cycle_upscale_mode(mode);
        ok &= require(mode == noctis::UpscaleMode::crisp_pixel, "cycle back to crisp_pixel");

        ok &= require(std::strlen(noctis::upscale_mode_name(noctis::UpscaleMode::crisp_pixel)) > 0, "name crisp");
        ok &= require(std::strlen(noctis::upscale_mode_name(noctis::UpscaleMode::edge_scale2x)) > 0, "name scale2x");
        ok &= require(std::strlen(noctis::upscale_mode_name(noctis::UpscaleMode::smooth_bilinear)) > 0, "name smooth");
    }

    // 2. Global state getter/setter
    {
        noctis::set_upscale_mode(noctis::UpscaleMode::edge_scale2x);
        ok &= require(noctis::get_upscale_mode() == noctis::UpscaleMode::edge_scale2x, "set/get upscale mode");
        noctis::set_upscale_mode(noctis::UpscaleMode::crisp_pixel);
    }

    // 3. Flat image scaling preserves all pixels
    {
        constexpr int w = 4;
        constexpr int h = 4;
        std::vector<std::uint32_t> src(w * h, 0xFF112233);
        std::vector<std::uint32_t> dst((w * 2) * (h * 2), 0);

        noctis::scale2x_rgba(src.data(), w, h, dst.data());

        for (std::size_t i = 0; i < dst.size(); ++i) {
            ok &= require(dst[i] == 0xFF112233, "flat image pixel preserved");
        }
    }

    // 4. Diagonal edge smoothing test
    // 3x3 pattern:
    //   0 1 0
    //   1 1 0
    //   0 0 0
    // Center pixel (1, 1) has: P=1, Above=1, Right=0, Left=1, Below=0
    // For (1, 1): C(left)=1, A(above)=1, D(below)=0, B(right)=0
    // Top-left E0: C==A (1==1) && C!=D (1!=0) && A!=B (1!=0) -> 1 (color of A/C)
    // Top-right E1: A==B is false (1!=0) -> P=1
    // Bottom-left E2: D==C is false (0!=1) -> P=1
    // Bottom-right E3: B==D (0==0) && B!=A (0!=1) && D!=C (0!=1) -> 0 (color of B/D)
    // Thus the outer corner (bottom-right) of the 2x2 expands to 0, smoothing the diagonal!
    {
        constexpr int w = 3;
        constexpr int h = 3;
        const std::uint32_t c0 = 0x00000000;
        const std::uint32_t c1 = 0xFFFFFFFF;
        const std::vector<std::uint32_t> src = {
            c0, c1, c0,
            c1, c1, c0,
            c0, c0, c0
        };
        std::vector<std::uint32_t> dst(6 * 6, 0);
        noctis::scale2x_rgba(src.data(), w, h, dst.data());

        // Check center pixel subpixels at dst x in [2, 3], y in [2, 3]
        const std::uint32_t e0 = dst[2 * 6 + 2]; // top-left
        const std::uint32_t e1 = dst[2 * 6 + 3]; // top-right
        const std::uint32_t e2 = dst[3 * 6 + 2]; // bottom-left
        const std::uint32_t e3 = dst[3 * 6 + 3]; // bottom-right

        ok &= require(e0 == c1, "diagonal smoothing E0 is foreground");
        ok &= require(e1 == c1, "diagonal smoothing E1 is foreground");
        ok &= require(e2 == c1, "diagonal smoothing E2 is foreground");
        ok &= require(e3 == c0, "diagonal corner E3 rounded to background");
    }

    // 5. Zero palette leakage (strictly palette-exact, no intermediate blending)
    {
        constexpr int w = 32;
        constexpr int h = 20;
        std::vector<std::uint32_t> src(w * h);
        std::unordered_set<std::uint32_t> original_colors;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Checkerboard and stripes
                const std::uint32_t color = ((x + y) % 3 == 0) ? 0xFF0000FF :
                                            ((x + y) % 3 == 1) ? 0xFF00FF00 : 0xFFFF0000;
                src[y * w + x] = color;
                original_colors.insert(color);
            }
        }

        std::vector<std::uint32_t> dst((w * 2) * (h * 2), 0);
        noctis::scale2x_rgba(src.data(), w, h, dst.data());

        bool leakage = false;
        for (const auto pix : dst) {
            if (original_colors.find(pix) == original_colors.end()) {
                leakage = true;
                break;
            }
        }
        ok &= require(!leakage, "zero palette leakage in scale2x");
    }

    // 6. Strict Determinism (two runs produce bit-for-bit identical results)
    {
        constexpr int w = 64;
        constexpr int h = 40;
        std::vector<std::uint32_t> src(w * h);
        for (int i = 0; i < w * h; ++i) {
            src[i] = static_cast<std::uint32_t>(i * 2654435761u);
        }

        std::vector<std::uint32_t> dst1((w * 2) * (h * 2));
        std::vector<std::uint32_t> dst2((w * 2) * (h * 2));

        noctis::scale2x_rgba(src.data(), w, h, dst1.data());
        noctis::scale2x_rgba(src.data(), w, h, dst2.data());

        ok &= require(std::memcmp(dst1.data(), dst2.data(), dst1.size() * sizeof(std::uint32_t)) == 0,
                      "scale2x must be 100% deterministic");
    }

    // 7. Indexed Scale2x equivalence
    {
        constexpr int w = 8;
        constexpr int h = 8;
        std::vector<std::uint8_t> src_idx(w * h);
        std::vector<std::uint32_t> src_rgba(w * h);
        for (int i = 0; i < w * h; ++i) {
            src_idx[i] = static_cast<std::uint8_t>((i * 7) % 16);
            src_rgba[i] = static_cast<std::uint32_t>(src_idx[i]);
        }

        std::vector<std::uint8_t> dst_idx((w * 2) * (h * 2));
        std::vector<std::uint32_t> dst_rgba((w * 2) * (h * 2));

        noctis::scale2x_indexed(src_idx.data(), w, h, dst_idx.data());
        noctis::scale2x_rgba(src_rgba.data(), w, h, dst_rgba.data());

        bool match = true;
        for (std::size_t i = 0; i < dst_idx.size(); ++i) {
            if (dst_idx[i] != static_cast<std::uint8_t>(dst_rgba[i])) {
                match = false;
                break;
            }
        }
        ok &= require(match, "scale2x indexed matches scale2x rgba");
    }

    // 8. Null and defensive bounds checks
    {
        noctis::scale2x_rgba(nullptr, 10, 10, nullptr);
        noctis::scale2x_rgba(nullptr, 0, 0, nullptr);
        noctis::scale2x_indexed(nullptr, 10, 10, nullptr);
        noctis::scale2x_indexed(nullptr, 0, 0, nullptr);
    }

    if (ok) {
        std::printf("upscale_tests: All tests passed successfully.\n");
        return 0;
    }
    return 1;
}
