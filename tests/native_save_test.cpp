#include "engine_state.h"
#include "native_save.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

bool require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

noctis::NativeSaveState distinctive_state() {
    noctis::NativeSaveState state;
    state.nsync = 5;
    state.anti_rad = 0;
    state.charge = 47;
    state.ip_targetted = 3;
    state.sys = 2;
    state.pwr = 17321;
    state.nearstar_nop = 5;
    state.pos_x = -1234.5F;
    state.pos_y = 0.25F;
    state.pos_z = 98765.0F;
    state.user_alfa = 179.75F;
    state.nearstar_ray = 5.020999908447266F;
    state.dzat_x = -18928.125;
    state.dzat_y = -29680.5;
    state.dzat_z = -67336.875;
    state.nearstar_x = -18928.0;
    state.nearstar_y = -29680.0;
    state.nearstar_z = -67336.0;
    state.reaction_time = 0.03125;
    state.fcs_status = {'A', 'P', 'P', 'R', 'O', 'A', 'C', 'H', 0, 0, 0};
    state.fcs_status_delay = -321;
    state.requested_vimana_coefficient = 65536.125;
    state.lithium_collector = 1;
    state.ap_reached = 1;
    state.lifter = -750;
    state.secs = 123456789.125;
    state.surlight = 31;
    state.gnc_pos = 7;
    state.goesfile_pos = 123456;
    state.goesnet_command.fill(0);
    constexpr char command[] = "CAST FELYSIA";
    std::copy(std::begin(command), std::end(command), state.goesnet_command.begin());
    state.last_snapshot = 7654321;
    state.option_mouse_look = 2;
    state.roof_speed = 1;
    state.hud_closed = 0;
    state.draw_hud = 0;
    state.lens_flare_mode = -1;
    state.seamless_border = 1;
    return state;
}

std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const auto byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const auto mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

void repair_crc(std::vector<std::uint8_t> &bytes) {
    const auto crc = crc32(std::span(bytes).subspan(20));
    for (std::size_t byte = 0; byte < 4; ++byte) bytes[16 + byte] = static_cast<std::uint8_t>(crc >> (byte * 8U));
}

std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

} // namespace

int main() {
    const auto original = distinctive_state();
    const auto bytes = noctis::encode_native_save(original);
    if (!require(bytes.size() == 401, "native v1 save must be exactly 401 bytes")
        || !require(bytes[0] == 'N' && bytes[7] == 0, "native save magic is wrong")
        || !require(bytes[8] == 1 && bytes[9] == 0, "native save version is not little-endian v1")) {
        return 1;
    }

    noctis::NativeSaveState decoded;
    const auto decoded_result = noctis::decode_native_save(bytes, decoded);
    if (!require(decoded_result.status == noctis::NativeSaveStatus::ok, "valid native save did not decode")
        || !require(noctis::encode_native_save(decoded) == bytes, "native save did not round-trip bit-exactly")) {
        return 1;
    }

    noctis::GoesTerminalState terminal;
    noctis::restore_goes_terminal_state(decoded, terminal);
    auto adapted = decoded;
    adapted.gnc_pos = 0;
    adapted.goesfile_pos = 0;
    adapted.goesnet_command = {};
    noctis::capture_goes_terminal_state(terminal, adapted);
    if (!require(noctis::encode_native_save(adapted) == bytes,
                 "terminal state adapter changed native v1 bytes")) {
        return 1;
    }

    auto future = bytes;
    future[8] = 2;
    if (!require(noctis::decode_native_save(future, decoded).status == noctis::NativeSaveStatus::unsupported_version,
                 "future native save version was not rejected")) {
        return 1;
    }
    auto damaged = bytes;
    damaged.back() ^= 0x80;
    if (!require(noctis::decode_native_save(damaged, decoded).status == noctis::NativeSaveStatus::invalid,
                 "damaged native save was not rejected")) {
        return 1;
    }
    auto truncated = bytes;
    truncated.pop_back();
    if (!require(noctis::decode_native_save(truncated, decoded).status == noctis::NativeSaveStatus::invalid,
                 "truncated native save was not rejected")) {
        return 1;
    }

    const auto guard = distinctive_state();
    for (std::size_t byte = 0; byte < bytes.size(); ++byte) {
        auto corrupt = bytes;
        corrupt[byte] ^= 1;
        auto unchanged = guard;
        if (!require(noctis::decode_native_save(corrupt, unchanged).status != noctis::NativeSaveStatus::ok,
                     "single-byte native corruption was accepted")
            || !require(noctis::encode_native_save(unchanged) == noctis::encode_native_save(guard),
                        "failed native decode partially changed its output")) {
            return 1;
        }
    }

    constexpr std::size_t payload = 20;
    for (const auto mutation : {0, 1, 2, 3, 4}) {
        auto semantic = bytes;
        if (mutation == 0) {
            semantic[payload + 25] = 0; // sys selector
            semantic[payload + 26] = 0;
        } else if (mutation == 1) {
            semantic[payload + 39] = 0; // pos_x = quiet NaN
            semantic[payload + 40] = 0;
            semantic[payload + 41] = 0xC0;
            semantic[payload + 42] = 0x7F;
        } else if (mutation == 2) {
            std::fill_n(semantic.begin() + payload + 183, 11, 'X'); // unterminated FCS status
        } else if (mutation == 3) {
            std::fill_n(semantic.begin() + payload + 250, 120, 'X'); // unterminated command
        } else {
            semantic[payload + 374] = 3; // invalid mouselook preference
        }
        repair_crc(semantic);
        auto unchanged = guard;
        if (!require(noctis::decode_native_save(semantic, unchanged).status == noctis::NativeSaveStatus::invalid,
                     "checksum-valid semantic corruption was accepted")
            || !require(noctis::encode_native_save(unchanged) == noctis::encode_native_save(guard),
                        "semantic rejection partially changed its output")) {
            return 1;
        }
    }

    noctis::SurfaceSaveState surface;
    surface.landing_longitude = 123;
    surface.landing_latitude = 45;
    surface.atl_x = 101;
    surface.atl_z = -202;
    surface.pos_x = 1638400.5F;
    surface.pos_y = -120.25F;
    surface.pos_z = 8192.0F;
    surface.user_alfa = -15.0F;
    surface.user_beta = 92.5F;
    surface.openhudcount = 75;
    surface.hud_rtl_closed = 0;
    const auto surface_bytes = noctis::encode_surface_save(surface);
    noctis::SurfaceSaveState decoded_surface;
    if (!require(surface_bytes.size() == 65, "native surface v1 save must be exactly 65 bytes")
        || !require(noctis::decode_surface_save(surface_bytes, decoded_surface).status
                        == noctis::NativeSaveStatus::ok,
                    "valid native surface save did not decode")
        || !require(noctis::encode_surface_save(decoded_surface) == surface_bytes,
                    "native surface save did not round-trip bit-exactly")) {
        return 1;
    }
    for (std::size_t byte = 0; byte < surface_bytes.size(); ++byte) {
        auto corrupt = surface_bytes;
        corrupt[byte] ^= 1;
        auto unchanged = surface;
        if (!require(noctis::decode_surface_save(corrupt, unchanged).status != noctis::NativeSaveStatus::ok,
                     "single-byte surface corruption was accepted")
            || !require(noctis::encode_surface_save(unchanged) == surface_bytes,
                        "failed surface decode partially changed its output")) {
            return 1;
        }
    }
    auto semantic_surface = surface_bytes;
    semantic_surface[payload + 20] = 0;
    semantic_surface[payload + 21] = 0;
    semantic_surface[payload + 22] = 0xC0;
    semantic_surface[payload + 23] = 0x7F;
    repair_crc(semantic_surface);
    auto unchanged_surface = surface;
    if (!require(noctis::decode_surface_save(semantic_surface, unchanged_surface).status
                     == noctis::NativeSaveStatus::invalid,
                 "checksum-valid surface NaN was accepted")
        || !require(noctis::encode_surface_save(unchanged_surface) == surface_bytes,
                    "surface semantic rejection partially changed its output")) {
        return 1;
    }

    const auto path = std::filesystem::current_path() / "native-save-test.niv";
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    const auto save_result = noctis::save_native_save(path, original);
    noctis::NativeSaveState loaded;
    const auto load_result = noctis::load_native_save(path, loaded);
    const bool file_ok = require(save_result.status == noctis::NativeSaveStatus::ok, "native save file write failed")
        && require(load_result.status == noctis::NativeSaveStatus::ok, "native save file read failed")
        && require(noctis::encode_native_save(loaded) == bytes, "native save file changed state")
        && require(!std::filesystem::exists(path.string() + ".tmp"), "temporary save file was left behind");
    const auto original_file = read_file(path);
    std::filesystem::create_directory(path.string() + ".tmp");
    auto replacement = original;
    replacement.pwr = 19000;
    const auto interrupted = noctis::save_native_save(path, replacement);
    const bool interrupted_ok = require(interrupted.status == noctis::NativeSaveStatus::io_error,
                                        "blocked temporary save was not reported")
        && require(read_file(path) == original_file, "blocked replacement changed the committed save")
        && require(std::filesystem::is_directory(path.string() + ".tmp"),
                   "writer removed a non-temporary directory");
    std::filesystem::remove_all(path.string() + ".tmp", ignored);

    std::filesystem::permissions(path, std::filesystem::perms::owner_read,
                                 std::filesystem::perm_options::replace);
    const auto readonly = noctis::save_native_save(path, replacement);
    const bool readonly_ok = require(readonly.status == noctis::NativeSaveStatus::io_error,
                                     "read-only native save was replaced")
        && require(read_file(path) == original_file, "read-only save changed after failed replacement");
    std::filesystem::permissions(path, std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::replace);

    const auto oversized = path.string() + ".oversized";
    {
        std::ofstream output(oversized, std::ios::binary | std::ios::trunc);
        output.seekp(1024 * 1024);
        output.put(0);
    }
    auto oversized_state = guard;
    const bool oversized_ok = require(noctis::load_native_save(oversized, oversized_state).status
                                           == noctis::NativeSaveStatus::invalid,
                                       "oversized native save was accepted")
        && require(noctis::encode_native_save(oversized_state) == noctis::encode_native_save(guard),
                   "oversized save load partially changed its output");

    std::filesystem::remove(oversized, ignored);
    std::filesystem::remove(path, ignored);
    return file_ok && interrupted_ok && readonly_ok && oversized_ok ? 0 : 1;
}
