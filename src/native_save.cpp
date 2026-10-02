#include "native_save.h"

#include "atomic_file.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <system_error>
#include <type_traits>

namespace noctis {
namespace {

constexpr std::array<std::uint8_t, 8> save_magic{'N', 'I', 'V', 'S', 'A', 'V', 'E', 0};
constexpr std::array<std::uint8_t, 8> surface_magic{'N', 'I', 'V', 'S', 'U', 'R', 'F', 0};
constexpr std::uint16_t header_size = 20;
constexpr std::uint32_t payload_size = 381;
constexpr std::uint32_t surface_payload_size = 45;
constexpr std::uintmax_t maximum_save_size = 1024 * 1024;

template <typename T>
void append_integer(std::vector<std::uint8_t> &bytes, T value) {
    using U = std::make_unsigned_t<T>;
    U bits = static_cast<U>(value);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        bytes.push_back(static_cast<std::uint8_t>(bits >> (i * 8U)));
    }
}

void append_float(std::vector<std::uint8_t> &bytes, float value) {
    append_integer(bytes, std::bit_cast<std::uint32_t>(value));
}

void append_double(std::vector<std::uint8_t> &bytes, double value) {
    append_integer(bytes, std::bit_cast<std::uint64_t>(value));
}

template <typename T>
bool read_integer(std::span<const std::uint8_t> bytes, std::size_t &offset, T &value) {
    if (bytes.size() - offset < sizeof(T)) {
        return false;
    }
    using U = std::make_unsigned_t<T>;
    U bits = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        bits |= static_cast<U>(bytes[offset++]) << (i * 8U);
    }
    if constexpr (std::is_signed_v<T>) {
        value = std::bit_cast<T>(bits);
    } else {
        value = bits;
    }
    return true;
}

bool read_float(std::span<const std::uint8_t> bytes, std::size_t &offset, float &value) {
    std::uint32_t bits;
    if (!read_integer(bytes, offset, bits)) {
        return false;
    }
    value = std::bit_cast<float>(bits);
    return true;
}

bool read_double(std::span<const std::uint8_t> bytes, std::size_t &offset, double &value) {
    std::uint64_t bits;
    if (!read_integer(bytes, offset, bits)) {
        return false;
    }
    value = std::bit_cast<double>(bits);
    return true;
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

void append_payload(std::vector<std::uint8_t> &bytes, const NativeSaveState &s) {
#define APPEND_I(name) append_integer(bytes, s.name)
    APPEND_I(nsync); APPEND_I(anti_rad); APPEND_I(pl_search); APPEND_I(field_amplificator);
    APPEND_I(ilight); APPEND_I(ilightv); APPEND_I(charge); APPEND_I(revcontrols);
    APPEND_I(ap_targetting); APPEND_I(ap_targetted); APPEND_I(ip_targetting); APPEND_I(ip_targetted);
    APPEND_I(ip_reaching); APPEND_I(ip_reached); APPEND_I(ap_target_spin); APPEND_I(ap_target_r);
    APPEND_I(ap_target_g); APPEND_I(ap_target_b); APPEND_I(nearstar_spin); APPEND_I(nearstar_r);
    APPEND_I(nearstar_g); APPEND_I(nearstar_b); APPEND_I(gburst); APPEND_I(menusalwayson); APPEND_I(depolarize);
    APPEND_I(sys); APPEND_I(pwr); APPEND_I(dev_page); APPEND_I(ap_target_class); APPEND_I(f_ray_elapsed);
    APPEND_I(nearstar_class); APPEND_I(nearstar_nop);
#undef APPEND_I
#define APPEND_F(name) append_float(bytes, s.name)
    APPEND_F(pos_x); APPEND_F(pos_y); APPEND_F(pos_z); APPEND_F(user_alfa); APPEND_F(user_beta);
    APPEND_F(navigation_beta); APPEND_F(ap_target_ray); APPEND_F(nearstar_ray);
#undef APPEND_F
#define APPEND_D(name) append_double(bytes, s.name)
    APPEND_D(dzat_x); APPEND_D(dzat_y); APPEND_D(dzat_z); APPEND_D(ap_target_x); APPEND_D(ap_target_y);
    APPEND_D(ap_target_z); APPEND_D(nearstar_x); APPEND_D(nearstar_y); APPEND_D(nearstar_z); APPEND_D(helptime);
    APPEND_D(ip_target_initial_d); APPEND_D(requested_approach_coefficient); APPEND_D(current_approach_coefficient);
    APPEND_D(reaction_time);
#undef APPEND_D
    bytes.insert(bytes.end(), s.fcs_status.begin(), s.fcs_status.end());
    append_integer(bytes, s.fcs_status_delay);
    append_integer(bytes, s.psys);
    append_double(bytes, s.ap_target_initial_d);
    append_double(bytes, s.requested_vimana_coefficient);
    append_double(bytes, s.current_vimana_coefficient);
    append_double(bytes, s.vimana_reaction_time);
    append_integer(bytes, s.lithium_collector);
    append_integer(bytes, s.autoscreenoff);
    append_integer(bytes, s.ap_reached);
    append_integer(bytes, s.lifter);
    append_double(bytes, s.secs);
    append_integer(bytes, s.data);
    append_integer(bytes, s.surlight);
    append_integer(bytes, s.gnc_pos);
    append_integer(bytes, s.goesfile_pos);
    bytes.insert(bytes.end(), s.goesnet_command.begin(), s.goesnet_command.end());
    append_integer(bytes, s.last_snapshot);
    append_integer(bytes, s.option_mouse_look);
    append_integer(bytes, s.roof_speed);
    append_integer(bytes, s.hud_closed);
    append_integer(bytes, s.draw_hud);
    append_integer(bytes, s.lens_flare_mode);
    append_integer(bytes, s.seamless_border);
}

bool read_payload(std::span<const std::uint8_t> bytes, NativeSaveState &s) {
    std::size_t offset = 0;
#define READ_I(name) if (!read_integer(bytes, offset, s.name)) return false
    READ_I(nsync); READ_I(anti_rad); READ_I(pl_search); READ_I(field_amplificator);
    READ_I(ilight); READ_I(ilightv); READ_I(charge); READ_I(revcontrols);
    READ_I(ap_targetting); READ_I(ap_targetted); READ_I(ip_targetting); READ_I(ip_targetted);
    READ_I(ip_reaching); READ_I(ip_reached); READ_I(ap_target_spin); READ_I(ap_target_r);
    READ_I(ap_target_g); READ_I(ap_target_b); READ_I(nearstar_spin); READ_I(nearstar_r);
    READ_I(nearstar_g); READ_I(nearstar_b); READ_I(gburst); READ_I(menusalwayson); READ_I(depolarize);
    READ_I(sys); READ_I(pwr); READ_I(dev_page); READ_I(ap_target_class); READ_I(f_ray_elapsed);
    READ_I(nearstar_class); READ_I(nearstar_nop);
#undef READ_I
#define READ_F(name) if (!read_float(bytes, offset, s.name)) return false
    READ_F(pos_x); READ_F(pos_y); READ_F(pos_z); READ_F(user_alfa); READ_F(user_beta);
    READ_F(navigation_beta); READ_F(ap_target_ray); READ_F(nearstar_ray);
#undef READ_F
#define READ_D(name) if (!read_double(bytes, offset, s.name)) return false
    READ_D(dzat_x); READ_D(dzat_y); READ_D(dzat_z); READ_D(ap_target_x); READ_D(ap_target_y);
    READ_D(ap_target_z); READ_D(nearstar_x); READ_D(nearstar_y); READ_D(nearstar_z); READ_D(helptime);
    READ_D(ip_target_initial_d); READ_D(requested_approach_coefficient); READ_D(current_approach_coefficient);
    READ_D(reaction_time);
#undef READ_D
    if (bytes.size() - offset < s.fcs_status.size()) return false;
    for (auto &value : s.fcs_status) value = static_cast<std::int8_t>(bytes[offset++]);
    if (!read_integer(bytes, offset, s.fcs_status_delay) || !read_integer(bytes, offset, s.psys)
        || !read_double(bytes, offset, s.ap_target_initial_d)
        || !read_double(bytes, offset, s.requested_vimana_coefficient)
        || !read_double(bytes, offset, s.current_vimana_coefficient)
        || !read_double(bytes, offset, s.vimana_reaction_time)
        || !read_integer(bytes, offset, s.lithium_collector)
        || !read_integer(bytes, offset, s.autoscreenoff)
        || !read_integer(bytes, offset, s.ap_reached)
        || !read_integer(bytes, offset, s.lifter)
        || !read_double(bytes, offset, s.secs)
        || !read_integer(bytes, offset, s.data)
        || !read_integer(bytes, offset, s.surlight)
        || !read_integer(bytes, offset, s.gnc_pos)
        || !read_integer(bytes, offset, s.goesfile_pos)
        || bytes.size() - offset < s.goesnet_command.size()) {
        return false;
    }
    for (auto &value : s.goesnet_command) value = static_cast<char>(bytes[offset++]);
    if (!read_integer(bytes, offset, s.last_snapshot)
        || !read_integer(bytes, offset, s.option_mouse_look)
        || !read_integer(bytes, offset, s.roof_speed)
        || !read_integer(bytes, offset, s.hud_closed)
        || !read_integer(bytes, offset, s.draw_hud)
        || !read_integer(bytes, offset, s.lens_flare_mode)
        || !read_integer(bytes, offset, s.seamless_border)) {
        return false;
    }
    return offset == bytes.size();
}

std::vector<std::uint8_t> encode_envelope(const std::array<std::uint8_t, 8> &magic,
                                          std::span<const std::uint8_t> payload) {
    std::vector<std::uint8_t> bytes(magic.begin(), magic.end());
    bytes.reserve(header_size + payload.size());
    append_integer(bytes, native_save_version);
    append_integer(bytes, header_size);
    append_integer(bytes, static_cast<std::uint32_t>(payload.size()));
    append_integer(bytes, crc32(payload));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

NativeSaveResult failure(NativeSaveStatus status, std::string message);

template <typename T>
bool has_nul(std::span<const T> text) {
    return std::find(text.begin(), text.end(), '\0') != text.end();
}

NativeSaveResult validate_state(const NativeSaveState &state) {
    if (state.sys < 1 || state.sys > 4 || state.ip_targetted < -1 || state.ip_targetted >= 80
        || state.ap_target_class < 0 || state.ap_target_class >= 12
        || state.nearstar_class < 0 || state.nearstar_class >= 12
        || state.nearstar_nop < 0 || state.nearstar_nop >= 80) {
        return failure(NativeSaveStatus::invalid, "native save contains an out-of-range selector");
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
        return failure(NativeSaveStatus::invalid, "native save contains a non-finite number");
    }
    if (!has_nul(std::span(state.fcs_status.data(), state.fcs_status.size())) || state.gnc_pos < 0
        || static_cast<std::size_t>(state.gnc_pos) >= state.goesnet_command.size()
        || !has_nul(std::span(state.goesnet_command.data(), state.goesnet_command.size()))) {
        return failure(NativeSaveStatus::invalid, "native save contains an unterminated text field");
    }
    if (state.option_mouse_look < 0 || state.option_mouse_look > 2
        || (state.roof_speed != 0 && state.roof_speed != 1)
        || (state.hud_closed != 0 && state.hud_closed != 1)
        || (state.draw_hud != 0 && state.draw_hud != 1)
        || state.lens_flare_mode < -1 || state.lens_flare_mode > 1
        || (state.seamless_border != 0 && state.seamless_border != 1)) {
        return failure(NativeSaveStatus::invalid, "native save contains an out-of-range preference");
    }
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult validate_surface(const SurfaceSaveState &state) {
    const std::array<float, 5> floats{state.pos_x, state.pos_y, state.pos_z, state.user_alfa, state.user_beta};
    if (!std::all_of(floats.begin(), floats.end(), [](float value) { return std::isfinite(value); })) {
        return failure(NativeSaveStatus::invalid, "native surface save contains a non-finite number");
    }
    if (state.hud_rtl_closed != 0 && state.hud_rtl_closed != 1) {
        return failure(NativeSaveStatus::invalid, "native surface save contains an invalid HUD state");
    }
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult inspect_envelope(std::span<const std::uint8_t> bytes,
                                  const std::array<std::uint8_t, 8> &magic,
                                  std::uint32_t expected_payload_size,
                                  std::span<const std::uint8_t> &payload) {
    if (bytes.size() < header_size || !std::equal(magic.begin(), magic.end(), bytes.begin())) {
        return failure(NativeSaveStatus::invalid, "missing native save header");
    }
    std::size_t offset = magic.size();
    std::uint16_t version;
    std::uint16_t encoded_header_size;
    std::uint32_t encoded_payload_size;
    std::uint32_t encoded_crc;
    if (!read_integer(bytes, offset, version) || !read_integer(bytes, offset, encoded_header_size)
        || !read_integer(bytes, offset, encoded_payload_size) || !read_integer(bytes, offset, encoded_crc)) {
        return failure(NativeSaveStatus::invalid, "truncated native save header");
    }
    if (version != native_save_version) {
        return failure(NativeSaveStatus::unsupported_version, "unsupported native save version");
    }
    if (encoded_header_size != header_size || encoded_payload_size != expected_payload_size
        || bytes.size() != encoded_header_size + encoded_payload_size) {
        return failure(NativeSaveStatus::invalid, "native save size does not match its header");
    }
    payload = bytes.subspan(encoded_header_size, encoded_payload_size);
    if (crc32(payload) != encoded_crc) {
        return failure(NativeSaveStatus::invalid, "native save checksum mismatch");
    }
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult failure(NativeSaveStatus status, std::string message) {
    return {status, std::move(message)};
}

void remove_temporary_file(const std::filesystem::path &path) {
    std::error_code error;
    if (std::filesystem::is_regular_file(path, error) || std::filesystem::is_symlink(path, error)) {
        std::filesystem::remove(path, error);
    }
}

NativeSaveResult write_atomic(const std::filesystem::path &path, std::span<const std::uint8_t> bytes,
                              std::string_view label) {
    std::error_code error;
    const bool destination_exists = std::filesystem::exists(path, error);
    if (error) return failure(NativeSaveStatus::io_error, "could not inspect " + std::string(label));
    if (destination_exists) {
        if (error || !std::filesystem::is_regular_file(path, error)) {
            return failure(NativeSaveStatus::io_error, "could not replace " + std::string(label));
        }
        const auto permissions = std::filesystem::status(path, error).permissions();
        using perms = std::filesystem::perms;
        if (error || (permissions & (perms::owner_write | perms::group_write | perms::others_write)) == perms::none) {
            return failure(NativeSaveStatus::io_error, std::string(label) + " is read-only");
        }
    }
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output || !output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()))
            || !output.flush()) {
            remove_temporary_file(temporary);
            return failure(NativeSaveStatus::io_error, "could not write " + std::string(label));
        }
    }
    if (!atomic_replace(temporary, path)) {
        remove_temporary_file(temporary);
        return failure(NativeSaveStatus::io_error, "could not replace " + std::string(label));
    }
    return {NativeSaveStatus::ok, {}};
}

} // namespace

std::vector<std::uint8_t> encode_native_save(const NativeSaveState &state) {
    std::vector<std::uint8_t> payload;
    payload.reserve(payload_size);
    append_payload(payload, state);

    return encode_envelope(save_magic, payload);
}

NativeSaveResult decode_native_save(std::span<const std::uint8_t> bytes, NativeSaveState &state) {
    std::span<const std::uint8_t> payload;
    const auto envelope = inspect_envelope(bytes, save_magic, payload_size, payload);
    if (envelope.status != NativeSaveStatus::ok) return envelope;
    NativeSaveState decoded;
    if (!read_payload(payload, decoded)) {
        return failure(NativeSaveStatus::invalid, "native save payload is malformed");
    }
    const auto validation = validate_state(decoded);
    if (validation.status != NativeSaveStatus::ok) return validation;
    state = decoded;
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult load_native_save(const std::filesystem::path &path, NativeSaveState &state) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::error_code error;
        if (!std::filesystem::exists(path, error) && !error) {
            return {NativeSaveStatus::not_found, {}};
        }
        return failure(NativeSaveStatus::io_error, "could not open native save");
    }
    std::error_code size_error;
    const auto size = std::filesystem::file_size(path, size_error);
    if (size_error) {
        return failure(NativeSaveStatus::io_error, "could not inspect native save");
    }
    if (size > maximum_save_size) {
        return failure(NativeSaveStatus::invalid, "native save exceeds the size limit");
    }
    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) {
        return failure(NativeSaveStatus::io_error, "could not read native save");
    }
    return decode_native_save(bytes, state);
}

NativeSaveResult save_native_save(const std::filesystem::path &path, const NativeSaveState &state) {
    const auto validation = validate_state(state);
    if (validation.status != NativeSaveStatus::ok) return validation;
    const auto bytes = encode_native_save(state);
    return write_atomic(path, bytes, "native save");
}

std::vector<std::uint8_t> encode_surface_save(const SurfaceSaveState &state) {
    std::vector<std::uint8_t> payload;
    payload.reserve(surface_payload_size);
    append_integer(payload, state.landing_longitude);
    append_integer(payload, state.landing_latitude);
    append_integer(payload, state.atl_x);
    append_integer(payload, state.atl_z);
    append_integer(payload, state.atl_x2);
    append_integer(payload, state.atl_z2);
    append_float(payload, state.pos_x);
    append_float(payload, state.pos_y);
    append_float(payload, state.pos_z);
    append_float(payload, state.user_alfa);
    append_float(payload, state.user_beta);
    append_integer(payload, state.openhuddelta);
    append_integer(payload, state.openhudcount);
    append_integer(payload, state.hud_rtl_closed);
    return encode_envelope(surface_magic, payload);
}

NativeSaveResult decode_surface_save(std::span<const std::uint8_t> bytes, SurfaceSaveState &state) {
    std::span<const std::uint8_t> payload;
    const auto envelope = inspect_envelope(bytes, surface_magic, surface_payload_size, payload);
    if (envelope.status != NativeSaveStatus::ok) return envelope;
    SurfaceSaveState decoded;
    std::size_t offset = 0;
    if (!read_integer(payload, offset, decoded.landing_longitude)
        || !read_integer(payload, offset, decoded.landing_latitude)
        || !read_integer(payload, offset, decoded.atl_x)
        || !read_integer(payload, offset, decoded.atl_z)
        || !read_integer(payload, offset, decoded.atl_x2)
        || !read_integer(payload, offset, decoded.atl_z2)
        || !read_float(payload, offset, decoded.pos_x)
        || !read_float(payload, offset, decoded.pos_y)
        || !read_float(payload, offset, decoded.pos_z)
        || !read_float(payload, offset, decoded.user_alfa)
        || !read_float(payload, offset, decoded.user_beta)
        || !read_integer(payload, offset, decoded.openhuddelta)
        || !read_integer(payload, offset, decoded.openhudcount)
        || !read_integer(payload, offset, decoded.hud_rtl_closed)
        || offset != payload.size()) {
        return failure(NativeSaveStatus::invalid, "native surface payload is malformed");
    }
    const auto validation = validate_surface(decoded);
    if (validation.status != NativeSaveStatus::ok) return validation;
    state = decoded;
    return {NativeSaveStatus::ok, {}};
}

NativeSaveResult load_surface_save(const std::filesystem::path &path, SurfaceSaveState &state) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::error_code error;
        if (!std::filesystem::exists(path, error) && !error) return {NativeSaveStatus::not_found, {}};
        return failure(NativeSaveStatus::io_error, "could not open native surface save");
    }
    std::error_code size_error;
    const auto size = std::filesystem::file_size(path, size_error);
    if (size_error) return failure(NativeSaveStatus::io_error, "could not inspect native surface save");
    if (size > maximum_save_size) return failure(NativeSaveStatus::invalid, "native surface save exceeds the size limit");
    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) return failure(NativeSaveStatus::io_error, "could not read native surface save");
    return decode_surface_save(bytes, state);
}

NativeSaveResult save_surface_save(const std::filesystem::path &path, const SurfaceSaveState &state) {
    const auto validation = validate_surface(state);
    if (validation.status != NativeSaveStatus::ok) return validation;
    const auto bytes = encode_surface_save(state);
    return write_atomic(path, bytes, "native surface save");
}

} // namespace noctis
