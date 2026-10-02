#include "legacy_save.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace noctis {
namespace {

constexpr std::size_t envelope_size = 20;
constexpr std::size_t native_situation_payload_size = 381;
constexpr std::size_t native_surface_payload_size = 45;

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

void set_crc(std::vector<std::uint8_t> &envelope, std::size_t payload_size) {
    const auto crc = crc32(std::span(envelope).subspan(envelope_size, payload_size));
    for (std::size_t byte = 0; byte < 4; ++byte) {
        envelope[16 + byte] = static_cast<std::uint8_t>(crc >> (byte * 8U));
    }
}

NativeSaveResult invalid(std::string message) {
    return {NativeSaveStatus::invalid, std::move(message)};
}

template <typename T>
bool has_nul(std::span<const T> text) {
    return std::find(text.begin(), text.end(), '\0') != text.end();
}

NativeSaveResult validate_situation(const NativeSaveState &state, std::size_t size) {
    if (state.sys < 1 || state.sys > 4 || state.ip_targetted < -1 || state.ip_targetted >= 80
        || state.ap_target_class < 0 || state.ap_target_class >= 12
        || state.nearstar_class < 0 || state.nearstar_class >= 12 || state.nearstar_nop < 0
        || state.nearstar_nop >= 80) {
        return invalid("legacy situation contains an out-of-range selector");
    }
    const std::array<float, 8> floats{state.pos_x, state.pos_y, state.pos_z, state.user_alfa, state.user_beta,
                                      state.navigation_beta, state.ap_target_ray, state.nearstar_ray};
    const std::array<double, 18> doubles{
        state.dzat_x, state.dzat_y, state.dzat_z, state.ap_target_x, state.ap_target_y, state.ap_target_z,
        state.nearstar_x, state.nearstar_y, state.nearstar_z, state.helptime, state.ip_target_initial_d,
        state.requested_approach_coefficient, state.current_approach_coefficient, state.reaction_time,
        state.ap_target_initial_d, state.requested_vimana_coefficient, state.current_vimana_coefficient,
        state.vimana_reaction_time};
    if (!std::all_of(floats.begin(), floats.end(), [](float value) { return std::isfinite(value); })
        || !std::all_of(doubles.begin(), doubles.end(), [](double value) { return std::isfinite(value); })) {
        return invalid("legacy situation contains a non-finite number");
    }
    if (!has_nul(std::span(state.fcs_status.data(), state.fcs_status.size()))) {
        return invalid("legacy FCS status is not terminated");
    }
    if (size >= 370
        && (state.gnc_pos < 0 || static_cast<std::size_t>(state.gnc_pos) >= state.goesnet_command.size()
            || !has_nul(std::span(state.goesnet_command.data(), state.goesnet_command.size())))) {
        return invalid("legacy GOESnet command state is not bounded");
    }
    if (size >= 377 && (state.option_mouse_look < 0 || state.option_mouse_look > 2)) {
        return invalid("legacy mouselook preference is out of range");
    }
    if (size >= 377 && state.roof_speed != 0 && state.roof_speed != 1) {
        return invalid("legacy roof-speed preference is out of range");
    }
    if (size >= 378 && state.hud_closed != 0 && state.hud_closed != 1) {
        return invalid("legacy HUD preference is out of range");
    }
    if (size >= 379 && state.draw_hud != 0 && state.draw_hud != 1) {
        return invalid("legacy HUD-text preference is out of range");
    }
    if (size >= 380 && (state.lens_flare_mode < -1 || state.lens_flare_mode > 1)) {
        return invalid("legacy lens-flare preference is out of range");
    }
    if (size >= 381 && state.seamless_border != 0 && state.seamless_border != 1) {
        return invalid("legacy visual preference is out of range");
    }
    return {NativeSaveStatus::ok, {}};
}

template <typename Result, typename Importer>
NativeSaveResult load_bounded(const std::filesystem::path &path, std::size_t maximum_size, Result &result,
                              Importer importer, std::string_view label) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::error_code error;
        if (!std::filesystem::exists(path, error) && !error) return {NativeSaveStatus::not_found, {}};
        return {NativeSaveStatus::io_error, "could not open " + std::string(label)};
    }
    std::error_code size_error;
    const auto size = std::filesystem::file_size(path, size_error);
    if (size_error) return {NativeSaveStatus::io_error, "could not inspect " + std::string(label)};
    if (size > maximum_size) return invalid(std::string(label) + " exceeds its largest supported layout");
    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) return {NativeSaveStatus::io_error, "could not read " + std::string(label)};
    return importer(bytes, result);
}

} // namespace

NativeSaveResult import_legacy_situation(std::span<const std::uint8_t> bytes, LegacySituationImport &result) {
    LegacySituationLayout layout;
    switch (bytes.size()) {
    case 245: layout = LegacySituationLayout::vanilla_245; break;
    case 370: layout = LegacySituationLayout::lr_370; break;
    case 377: layout = LegacySituationLayout::nivplus_377; break;
    case 378: layout = LegacySituationLayout::nivplus_378; break;
    case 379: layout = LegacySituationLayout::nivplus_379; break;
    case 380: layout = LegacySituationLayout::nivplus_380; break;
    case 381: layout = LegacySituationLayout::nivplus_381; break;
    case 382: layout = LegacySituationLayout::nivplus_transitional_382; break;
    default: return invalid("unsupported legacy situation size");
    }

    auto normalized = encode_native_save(NativeSaveState{});
    if (layout == LegacySituationLayout::nivplus_transitional_382) {
        std::copy_n(bytes.begin(), 377, normalized.begin() + envelope_size);
        normalized[envelope_size + 377] = bytes[381];
    } else {
        std::copy(bytes.begin(), bytes.end(), normalized.begin() + envelope_size);
    }
    set_crc(normalized, native_situation_payload_size);
    NativeSaveState state;
    const auto decoded = decode_native_save(normalized, state);
    if (decoded.status != NativeSaveStatus::ok) return decoded;
    const auto validation = validate_situation(state, bytes.size());
    if (validation.status != NativeSaveStatus::ok) return validation;
    result = {state, layout};
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult load_legacy_situation(const std::filesystem::path &path, LegacySituationImport &result) {
    return load_bounded(path, 382, result, import_legacy_situation, "legacy situation save");
}

NativeSaveResult import_legacy_surface(std::span<const std::uint8_t> bytes, LegacySurfaceImport &result) {
    LegacySurfaceLayout layout;
    if (bytes.size() == 40) {
        layout = LegacySurfaceLayout::dos_40;
    } else if (bytes.size() == 45) {
        layout = LegacySurfaceLayout::lr_45;
    } else {
        return invalid("unsupported legacy surface size");
    }
    auto normalized = encode_surface_save(SurfaceSaveState{});
    std::copy(bytes.begin(), bytes.end(), normalized.begin() + envelope_size);
    set_crc(normalized, native_surface_payload_size);
    SurfaceSaveState state;
    const auto decoded = decode_surface_save(normalized, state);
    if (decoded.status != NativeSaveStatus::ok) return decoded;
    const std::array<float, 5> floats{state.pos_x, state.pos_y, state.pos_z, state.user_alfa, state.user_beta};
    if (!std::all_of(floats.begin(), floats.end(), [](float value) { return std::isfinite(value); })) {
        return invalid("legacy surface contains a non-finite number");
    }
    result = {state, layout};
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult load_legacy_surface(const std::filesystem::path &path, LegacySurfaceImport &result) {
    return load_bounded(path, native_surface_payload_size, result, import_legacy_surface, "legacy surface save");
}

NativeSaveResult load_or_migrate_surface(const std::filesystem::path &native_path,
                                         const std::filesystem::path &legacy_path,
                                         SurfaceRestore &result) {
    SurfaceSaveState native_state;
    const auto native_result = load_surface_save(native_path, native_state);
    if (native_result.status == NativeSaveStatus::ok) {
        result = {native_state, false, LegacySurfaceLayout::dos_40};
        return native_result;
    }
    if (native_result.status != NativeSaveStatus::not_found) return native_result;

    LegacySurfaceImport imported;
    const auto legacy_result = load_legacy_surface(legacy_path, imported);
    if (legacy_result.status != NativeSaveStatus::ok) return legacy_result;
    const auto migration_result = save_surface_save(native_path, imported.state);
    if (migration_result.status != NativeSaveStatus::ok) return migration_result;
    result = {imported.state, true, imported.layout};
    return {NativeSaveStatus::ok, {}};
}

std::string_view legacy_layout_name(LegacySituationLayout layout) {
    switch (layout) {
    case LegacySituationLayout::vanilla_245: return "vanilla-245";
    case LegacySituationLayout::lr_370: return "lr-370";
    case LegacySituationLayout::nivplus_377: return "nivplus-377";
    case LegacySituationLayout::nivplus_378: return "nivplus-378";
    case LegacySituationLayout::nivplus_379: return "nivplus-379";
    case LegacySituationLayout::nivplus_380: return "nivplus-380";
    case LegacySituationLayout::nivplus_381: return "nivplus-381";
    case LegacySituationLayout::nivplus_transitional_382: return "nivplus-transitional-382";
    }
    return "unknown";
}

std::string_view legacy_layout_name(LegacySurfaceLayout layout) {
    switch (layout) {
    case LegacySurfaceLayout::dos_40: return "dos-40";
    case LegacySurfaceLayout::lr_45: return "lr-45";
    }
    return "unknown";
}

} // namespace noctis
