#include "gallery.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::fprintf(stderr, "gallery: %s\n", message);
    return condition;
}

void write_u32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
}

// Same layout write_indexed_bmp and compose_panorama produce: bottom-up rows,
// 256-entry BGRA palette at byte 54, pixels at byte 1078.
std::vector<std::uint8_t> bitmap(std::uint32_t width, std::uint32_t height) {
    const auto stride = (width + 3) & ~3U;
    std::vector<std::uint8_t> bytes(1078 + stride * height, 0);
    bytes[0] = 'B'; bytes[1] = 'M';
    write_u32(bytes, 2, static_cast<std::uint32_t>(bytes.size()));
    write_u32(bytes, 10, 1078);
    write_u32(bytes, 14, 40);
    write_u32(bytes, 18, width);
    write_u32(bytes, 22, height);
    bytes[26] = 1;
    bytes[28] = 8;
    for (std::size_t color = 0; color < 256; ++color) {
        bytes[54 + color * 4 + 0] = static_cast<std::uint8_t>(color);       // blue
        bytes[54 + color * 4 + 1] = static_cast<std::uint8_t>(255 - color); // green
        bytes[54 + color * 4 + 2] = 7;                                      // red
    }
    // Index 1 in the stored first row (the image's bottom row); index 2 in the stored last row.
    bytes[1078] = 1;
    bytes[1078 + stride * (height - 1)] = 2;
    return bytes;
}

void write_file(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

noctis::InputFrame frame_with(void (*set)(noctis::InputFrame &)) {
    noctis::InputFrame frame;
    set(frame);
    return frame;
}
} // namespace

int main(int argc, char **argv) {
    using namespace noctis;
    if (argc != 2) {
        std::fputs("usage: gallery_test SCRATCH_DIR\n", stderr);
        return 2;
    }
    const std::filesystem::path root(argv[1]);
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(root);
    bool ok = true;

    ok &= require(scan_gallery(root / "missing").empty(), "missing directory is not empty");
    ok &= require(!find_gallery_entry({}, ""), "empty gallery resolved an entry");

    write_file(root / "00000002.BMP", bitmap(916, 200));
    write_file(root / "00000010.BMP", bitmap(320, 200));
    write_file(root / "SNAP0003.BMP", bitmap(320, 200));
    write_file(root / "00000011.bmp", bitmap(66, 64));
    write_file(root / "WIDE9998.BMP", bitmap(320, 200));
    write_file(root / "00000012.BMP", {'B', 'M', 0, 0});
    write_file(root / "NOTES.BMP", bitmap(320, 200));
    write_file(root / "00000013.PNG", bitmap(320, 200));

    const auto entries = scan_gallery(root);
    ok &= require(entries.size() == 4, "scan did not keep exactly the four valid images");
    if (entries.size() == 4) {
        ok &= require(entries[0].id == "SNAP0003" && entries[0].legacy, "legacy snapshot is not first");
        ok &= require(entries[1].id == "00000002" && entries[1].kind == GalleryImageKind::panorama
                          && entries[1].width == 916, "panorama entry mismatch");
        ok &= require(entries[2].id == "00000010" && entries[2].kind == GalleryImageKind::snapshot, "snapshot entry mismatch");
        ok &= require(entries[3].id == "00000011" && entries[3].kind == GalleryImageKind::other, "lowercase extension mismatch");

        ok &= require(find_gallery_entry(entries, "") == 3u, "empty key is not the newest");
        ok &= require(find_gallery_entry(entries, "10") == 2u, "short number lookup failed");
        ok &= require(find_gallery_entry(entries, "00000002") == 1u, "padded number lookup failed");
        ok &= require(find_gallery_entry(entries, "snap0003") == 0u, "legacy name lookup failed");
        ok &= require(!find_gallery_entry(entries, "3"), "number matched a legacy name");
        ok &= require(!find_gallery_entry(entries, "999"), "absent number resolved");
        ok &= require(!find_gallery_entry(entries, "123456789"), "nine-digit key resolved");

        const auto image = load_gallery_image(entries[1].path);
        ok &= require(image && image->width == 916 && image->height == 200
                          && image->rgba.size() == 916u * 200u * 4u, "panorama decode size mismatch");
        if (image) {
            const auto *top = image->rgba.data();
            const auto *bottom = image->rgba.data() + 199u * 916u * 4u;
            ok &= require(top[0] == 7 && top[1] == 253 && top[2] == 2 && top[3] == 255, "top row is not bottom-up flipped");
            ok &= require(bottom[0] == 7 && bottom[1] == 254 && bottom[2] == 1, "bottom row color mismatch");
            ok &= require(top[4] == 7 && top[5] == 255 && top[6] == 0, "palette index zero mismatch");
        }
        const auto padded = load_gallery_image(entries[3].path);
        ok &= require(padded && padded->width == 66 && padded->rgba[(63u * 66u) * 4u + 1u] == 254, "padded-stride image did not decode");
    }

    auto truncated = bitmap(320, 200);
    truncated.resize(truncated.size() - 1);
    write_file(root / "00000020.BMP", truncated);
    ok &= require(!load_gallery_image(root / "00000020.BMP"), "truncated pixels decoded");
    auto truecolor = bitmap(320, 200);
    truecolor[28] = 24;
    write_file(root / "00000021.BMP", truecolor);
    ok &= require(!load_gallery_image(root / "00000021.BMP"), "24-bit image decoded");
    ok &= require(!load_gallery_image(root / "absent.BMP"), "absent file decoded");

    GalleryViewerState state{true, 1, 3, false, 0.5F};
    ok &= require(apply_gallery_command(state, GalleryCommand::next) && state.index == 2, "next failed");
    ok &= require(!apply_gallery_command(state, GalleryCommand::next) && state.index == 2, "next passed the end");
    ok &= require(apply_gallery_command(state, GalleryCommand::first) && state.index == 0, "first failed");
    ok &= require(!apply_gallery_command(state, GalleryCommand::previous) && state.index == 0, "previous passed the start");
    ok &= require(apply_gallery_command(state, GalleryCommand::last) && state.index == 2, "last failed");
    apply_gallery_command(state, GalleryCommand::pan_left);
    ok &= require(state.pan == 0.5F, "pan moved while fitted");
    apply_gallery_command(state, GalleryCommand::toggle_zoom);
    for (int step = 0; step < 10; ++step) apply_gallery_command(state, GalleryCommand::pan_left);
    ok &= require(state.zoomed && state.pan == 0.0F, "pan did not clamp at the left edge");
    for (int step = 0; step < 10; ++step) apply_gallery_command(state, GalleryCommand::pan_right);
    ok &= require(state.pan == 1.0F, "pan did not clamp at the right edge");
    apply_gallery_command(state, GalleryCommand::previous);
    ok &= require(state.zoomed && state.pan == 0.5F, "browsing did not recenter the zoomed view");
    apply_gallery_command(state, GalleryCommand::close);
    ok &= require(!state.open, "close failed");
    ok &= require(!apply_gallery_command(state, GalleryCommand::next) && state.index == 1, "closed viewer moved");

    auto commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.arrow_left_pressed = true; }), false);
    ok &= require(commands.size() == 1 && commands[0] == GalleryCommand::previous, "Left does not browse when fitted");
    commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.arrow_left_pressed = true; }), true);
    ok &= require(commands.size() == 1 && commands[0] == GalleryCommand::pan_left, "Left does not pan when zoomed");
    commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.page_down_pressed = true; }), true);
    ok &= require(commands.size() == 1 && commands[0] == GalleryCommand::next, "Page Down does not browse when zoomed");
    commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.text = {'z'}; }), false);
    ok &= require(commands.size() == 1 && commands[0] == GalleryCommand::toggle_zoom, "z does not toggle zoom");
    commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.text = {' '}; f.space_pressed = true; }), false);
    ok &= require(commands.size() == 1, "Space toggled zoom twice");
    commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.escape_down = true; f.arrow_right_pressed = true; }), false);
    ok &= require(commands.size() == 1 && commands[0] == GalleryCommand::close, "Escape does not close exclusively");
    commands = gallery_commands_for_frame(frame_with([](InputFrame &f) { f.f4_pressed = true; }), false);
    ok &= require(commands.size() == 1 && commands[0] == GalleryCommand::close, "F4 does not close");
    ok &= require(gallery_commands_for_frame({}, false).empty(), "idle frame produced commands");

    std::filesystem::remove_all(root, ignored);
    if (ok) std::puts("gallery: ok");
    return ok ? 0 : 1;
}
