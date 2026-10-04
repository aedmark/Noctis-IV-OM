#include "goesnet_data.h"

#include "atomic_file.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>

namespace noctis {
namespace {
constexpr std::size_t header_size = 4;
constexpr std::size_t starmap_record_size = 32;
constexpr std::size_t guide_record_size = 84;
constexpr std::uintmax_t maximum_data_size = 64U * 1024U * 1024U;
constexpr std::array<std::uint8_t, 8> removed_marker{'R', 'e', 'm', 'o', 'v', 'e', 'd', ':'};

std::uint32_t read_u32(const std::uint8_t *bytes) {
    return static_cast<std::uint32_t>(bytes[0]) | static_cast<std::uint32_t>(bytes[1]) << 8U
        | static_cast<std::uint32_t>(bytes[2]) << 16U | static_cast<std::uint32_t>(bytes[3]) << 24U;
}

double read_f64(const std::uint8_t *bytes) {
    std::uint64_t bits = 0;
    for (unsigned shift = 0; shift < 64; shift += 8) bits |= static_cast<std::uint64_t>(*bytes++) << shift;
    return std::bit_cast<double>(bits);
}

void write_f64(std::uint8_t *bytes, double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    for (unsigned shift = 0; shift < 64; shift += 8) *bytes++ = static_cast<std::uint8_t>(bits >> shift);
}

bool is_removed(const std::uint8_t *bytes) {
    return std::equal(removed_marker.begin(), removed_marker.end(), bytes);
}

GoesDataResult failure(GoesDataStatus status, std::string message) {
    return {status, std::move(message)};
}

GoesDataResult read_file(const std::filesystem::path &path, std::vector<std::uint8_t> &bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::error_code error;
        if (!std::filesystem::exists(path, error) && !error) return failure(GoesDataStatus::not_found, "data file is missing");
        return failure(GoesDataStatus::io_error, "data file could not be opened");
    }
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) return failure(GoesDataStatus::io_error, "data file size could not be inspected");
    if (size > maximum_data_size) return failure(GoesDataStatus::corrupt, "data file exceeds the size limit");
    bytes.assign(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) return failure(GoesDataStatus::io_error, "data file could not be read");
    return {};
}

bool valid_boundary(std::uint32_t boundary, std::size_t size, std::size_t record_size) {
    return boundary >= header_size && boundary <= size && (boundary - header_size) % record_size == 0;
}

std::string trim_field(const std::uint8_t *bytes, std::size_t size) {
    while (size != 0 && (bytes[size - 1] == ' ' || bytes[size - 1] == 0)) --size;
    return std::string(reinterpret_cast<const char *>(bytes), size);
}

bool valid_message(std::string_view message) {
    return !message.empty() && message.size() <= 76
        && std::all_of(message.begin(), message.end(), [](unsigned char c) { return c >= 32 && c <= 126; });
}

bool has_write_permission(const std::filesystem::path &path) {
    std::error_code error;
    const auto permissions = std::filesystem::status(path, error).permissions();
    if (error) return false;
    using perms = std::filesystem::perms;
    return (permissions & (perms::owner_write | perms::group_write | perms::others_write)) != perms::none;
}

GoesDataResult write_guide(const std::filesystem::path &path, const GuideData &data) {
    if (!has_write_permission(path)) return failure(GoesDataStatus::io_error, "guide is read-only");
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        const std::array<std::uint8_t, 4> header{
            static_cast<std::uint8_t>(data.consolidated_size),
            static_cast<std::uint8_t>(data.consolidated_size >> 8U),
            static_cast<std::uint8_t>(data.consolidated_size >> 16U),
            static_cast<std::uint8_t>(data.consolidated_size >> 24U)};
        if (!output || !output.write(reinterpret_cast<const char *>(header.data()), header.size())) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return failure(GoesDataStatus::io_error, "guide temporary could not be written");
        }
        for (const auto &record : data.records) {
            if (!output.write(reinterpret_cast<const char *>(record.bytes.data()), record.bytes.size())) {
                std::error_code ignored;
                std::filesystem::remove(temporary, ignored);
                return failure(GoesDataStatus::io_error, "guide temporary could not be written");
            }
        }
        if (!output.flush()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return failure(GoesDataStatus::io_error, "guide temporary could not be flushed");
        }
    }
    if (!atomic_replace(temporary, path)) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return failure(GoesDataStatus::io_error, "guide could not be replaced");
    }
    return {};
}

GoesDataResult write_starmap(const std::filesystem::path &path, const StarmapData &data) {
    if (!has_write_permission(path)) return failure(GoesDataStatus::io_error, "starmap is read-only");
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        const std::array<std::uint8_t, 4> header{
            static_cast<std::uint8_t>(data.consolidated_size),
            static_cast<std::uint8_t>(data.consolidated_size >> 8U),
            static_cast<std::uint8_t>(data.consolidated_size >> 16U),
            static_cast<std::uint8_t>(data.consolidated_size >> 24U)};
        if (!output || !output.write(reinterpret_cast<const char *>(header.data()), header.size())) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return failure(GoesDataStatus::io_error, "starmap temporary could not be written");
        }
        for (const auto &record : data.records) {
            if (!output.write(reinterpret_cast<const char *>(record.bytes.data()), record.bytes.size())) {
                std::error_code ignored;
                std::filesystem::remove(temporary, ignored);
                return failure(GoesDataStatus::io_error, "starmap temporary could not be written");
            }
        }
        if (!output.flush()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return failure(GoesDataStatus::io_error, "starmap temporary could not be flushed");
        }
    }
    if (!atomic_replace(temporary, path)) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return failure(GoesDataStatus::io_error, "starmap could not be replaced");
    }
    return {};
}

void set_guide_message(GuideRecord &record, double subject_id, std::string_view message) {
    record.bytes.fill(0);
    write_f64(record.bytes.data(), subject_id);
    std::copy(message.begin(), message.end(), record.bytes.begin() + 8);
    record.subject_id = subject_id;
    record.message.assign(message);
    record.removed = false;
}
} // namespace

GoesDataResult load_starmap(const std::filesystem::path &path, StarmapData &data) {
    std::vector<std::uint8_t> bytes;
    auto result = read_file(path, bytes);
    if (result.status != GoesDataStatus::ok) return result;
    if (bytes.size() < header_size || (bytes.size() - header_size) % starmap_record_size != 0)
        return failure(GoesDataStatus::corrupt, "starmap size is not record-aligned");
    StarmapData decoded;
    decoded.consolidated_size = read_u32(bytes.data());
    if (!valid_boundary(decoded.consolidated_size, bytes.size(), starmap_record_size))
        return failure(GoesDataStatus::corrupt, "starmap boundary is invalid");
    decoded.records.reserve((bytes.size() - header_size) / starmap_record_size);
    for (std::size_t offset = header_size; offset < bytes.size(); offset += starmap_record_size) {
        StarmapRecord record;
        std::copy_n(bytes.data() + offset, record.bytes.size(), record.bytes.begin());
        record.removed = is_removed(record.bytes.data());
        record.protected_record = offset < decoded.consolidated_size;
        if (!record.removed) {
            record.id = read_f64(record.bytes.data());
            if (!std::isfinite(record.id)) return failure(GoesDataStatus::corrupt, "starmap object ID is invalid");
            record.name = trim_field(record.bytes.data() + 8, 20);
            const auto kind = record.bytes[29];
            const bool decimal_ordinal = record.bytes[30] >= '0' && record.bytes[30] <= '9'
                && record.bytes[31] >= '0' && record.bytes[31] <= '9';
            const bool negative_ordinal = record.bytes[30] == '-'
                && record.bytes[31] >= '0' && record.bytes[31] <= '9';
            if ((kind != 'S' && kind != 'P') || (!decimal_ordinal && !negative_ordinal))
                return failure(GoesDataStatus::corrupt, "starmap label is invalid");
            record.kind = kind == 'S' ? GoesObjectKind::star : GoesObjectKind::planet;
            record.ordinal = negative_ordinal ? -(record.bytes[31] - '0')
                                              : (record.bytes[30] - '0') * 10 + record.bytes[31] - '0';
        }
        decoded.records.push_back(std::move(record));
    }
    data = std::move(decoded);
    return {};
}

GoesDataResult load_guide(const std::filesystem::path &path, GuideData &data) {
    std::vector<std::uint8_t> bytes;
    auto result = read_file(path, bytes);
    if (result.status != GoesDataStatus::ok) return result;
    if (bytes.size() < header_size || (bytes.size() - header_size) % guide_record_size != 0)
        return failure(GoesDataStatus::corrupt, "guide size is not record-aligned");
    GuideData decoded;
    decoded.consolidated_size = read_u32(bytes.data());
    if (!valid_boundary(decoded.consolidated_size, bytes.size(), guide_record_size))
        return failure(GoesDataStatus::corrupt, "guide boundary is invalid");
    decoded.records.reserve((bytes.size() - header_size) / guide_record_size);
    for (std::size_t offset = header_size; offset < bytes.size(); offset += guide_record_size) {
        GuideRecord record;
        std::copy_n(bytes.data() + offset, record.bytes.size(), record.bytes.begin());
        record.removed = is_removed(record.bytes.data());
        record.protected_record = offset < decoded.consolidated_size;
        if (!record.removed) {
            record.subject_id = read_f64(record.bytes.data());
            if (!std::isfinite(record.subject_id)) return failure(GoesDataStatus::corrupt, "guide subject ID is invalid");
            const auto end = std::find(record.bytes.begin() + 8, record.bytes.end(), 0);
            record.message.assign(reinterpret_cast<const char *>(record.bytes.data() + 8),
                                  static_cast<std::size_t>(end - (record.bytes.begin() + 8)));
        }
        decoded.records.push_back(std::move(record));
    }
    data = std::move(decoded);
    return {};
}

std::vector<std::size_t> find_starmap_objects(const StarmapData &data, std::string_view key) {
    std::vector<std::size_t> matches;
    if (key.empty() || key.size() > 20) return matches;
    for (std::size_t index = 0; index < data.records.size(); ++index) {
        const auto &record = data.records[index];
        if (!record.removed && record.name.size() >= key.size()
            && std::equal(key.begin(), key.end(), record.name.begin())) matches.push_back(index);
    }
    const auto exact = std::find_if(matches.begin(), matches.end(), [&](std::size_t index) {
        return data.records[index].name == key;
    });
    if (exact != matches.end()) return {*exact};
    return matches;
}

std::optional<std::string> find_starmap_name_by_id(const StarmapData &data, double id) {
    for (const auto &record : data.records) {
        if (!record.removed && std::abs(record.id - id) <= goes_id_tolerance) {
            return record.name;
        }
    }
    return std::nullopt;
}

std::vector<std::size_t> guide_records_for(const GuideData &data, double subject_id) {
    std::vector<std::size_t> matches;
    for (std::size_t index = 0; index < data.records.size(); ++index) {
        const auto &record = data.records[index];
        if (!record.removed && std::abs(record.subject_id - subject_id) <= goes_id_tolerance) matches.push_back(index);
    }
    return matches;
}

GoesDataResult append_starmap_label(const std::filesystem::path &path, double id,
                                    std::string_view name, GoesObjectKind kind,
                                    std::int16_t ordinal, std::int32_t &byte_offset) {
    byte_offset = -1;
    if (!std::isfinite(id) || name.empty() || name.size() > 20 || ordinal < 0 || ordinal > 99
        || !std::all_of(name.begin(), name.end(), [](unsigned char c) { return c >= 32 && c <= 126; }))
        return failure(GoesDataStatus::rejected, "starmap label is invalid");
    StarmapData data;
    auto loaded = load_starmap(path, data);
    if (loaded.status != GoesDataStatus::ok) return loaded;
    if (std::any_of(data.records.begin(), data.records.end(), [&](const StarmapRecord &record) {
            return !record.removed && record.name == name;
        })) return failure(GoesDataStatus::rejected, "starmap label already exists");
    if (data.records.size() > (std::numeric_limits<std::int32_t>::max() - header_size) / starmap_record_size)
        return failure(GoesDataStatus::rejected, "starmap has too many records");
    StarmapRecord record;
    record.bytes.fill(' ');
    write_f64(record.bytes.data(), id);
    std::copy(name.begin(), name.end(), record.bytes.begin() + 8);
    record.bytes[29] = kind == GoesObjectKind::star ? 'S' : 'P';
    record.bytes[30] = static_cast<std::uint8_t>('0' + ordinal / 10);
    record.bytes[31] = static_cast<std::uint8_t>('0' + ordinal % 10);
    record.id = id;
    record.name.assign(name);
    record.kind = kind;
    record.ordinal = ordinal;
    record.protected_record = false;
    byte_offset = static_cast<std::int32_t>(header_size + data.records.size() * starmap_record_size);
    data.records.push_back(std::move(record));
    const auto written = write_starmap(path, data);
    if (written.status != GoesDataStatus::ok) byte_offset = -1;
    return written;
}

GoesDataResult remove_starmap_label(const std::filesystem::path &path, std::int32_t byte_offset) {
    if (byte_offset < static_cast<std::int32_t>(header_size)
        || (byte_offset - static_cast<std::int32_t>(header_size)) % starmap_record_size != 0)
        return failure(GoesDataStatus::rejected, "starmap label offset is invalid");
    StarmapData data;
    auto loaded = load_starmap(path, data);
    if (loaded.status != GoesDataStatus::ok) return loaded;
    const auto index = static_cast<std::size_t>(byte_offset - header_size) / starmap_record_size;
    if (index >= data.records.size() || data.records[index].removed)
        return failure(GoesDataStatus::rejected, "starmap label does not exist");
    if (data.records[index].protected_record)
        return failure(GoesDataStatus::rejected, "starmap label is protected");
    std::copy(removed_marker.begin(), removed_marker.end(), data.records[index].bytes.begin());
    data.records[index].removed = true;
    return write_starmap(path, data);
}

GoesDataResult append_guide_record(const std::filesystem::path &path, double subject_id, std::string_view message) {
    if (!std::isfinite(subject_id) || !valid_message(message)) return failure(GoesDataStatus::rejected, "guide note is invalid");
    GuideData data;
    auto result = load_guide(path, data);
    if (result.status != GoesDataStatus::ok) return result;
    GuideRecord record;
    set_guide_message(record, subject_id, message);
    data.records.push_back(std::move(record));
    return write_guide(path, data);
}

GoesDataResult replace_guide_record(const std::filesystem::path &path, double subject_id,
                                    std::size_t live_ordinal, std::string_view message) {
    if (live_ordinal == 0 || !valid_message(message)) return failure(GoesDataStatus::rejected, "guide replacement is invalid");
    GuideData data;
    auto result = load_guide(path, data);
    if (result.status != GoesDataStatus::ok) return result;
    const auto matches = guide_records_for(data, subject_id);
    if (live_ordinal > matches.size()) return failure(GoesDataStatus::rejected, "guide record does not exist");
    auto &record = data.records[matches[live_ordinal - 1]];
    if (record.protected_record) return failure(GoesDataStatus::rejected, "guide record is protected");
    set_guide_message(record, subject_id, message);
    return write_guide(path, data);
}

GoesDataResult delete_guide_records(const std::filesystem::path &path, double subject_id,
                                    std::size_t first, std::size_t last, std::size_t &removed,
                                    std::size_t &protected_records) {
    removed = 0;
    protected_records = 0;
    if (first == 0 || last < first) return failure(GoesDataStatus::rejected, "guide range is invalid");
    GuideData data;
    auto result = load_guide(path, data);
    if (result.status != GoesDataStatus::ok) return result;
    const auto matches = guide_records_for(data, subject_id);
    for (std::size_t ordinal = first; ordinal <= last && ordinal <= matches.size(); ++ordinal) {
        auto &record = data.records[matches[ordinal - 1]];
        if (record.protected_record) {
            ++protected_records;
        } else {
            std::copy(removed_marker.begin(), removed_marker.end(), record.bytes.begin());
            record.removed = true;
            ++removed;
        }
    }
    if (removed == 0) return failure(GoesDataStatus::rejected, "no mutable guide records selected");
    return write_guide(path, data);
}

} // namespace noctis
