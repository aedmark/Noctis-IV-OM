#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

constexpr double goes_id_tolerance = 0.00001;

enum class GoesDataStatus : std::uint8_t { ok, not_found, io_error, corrupt, rejected };
enum class GoesObjectKind : std::uint8_t { star, planet };

struct StarmapRecord {
    double id{};
    std::string name;
    GoesObjectKind kind = GoesObjectKind::star;
    std::int16_t ordinal{};
    bool removed{};
    bool protected_record{};
    std::array<std::uint8_t, 32> bytes{};
};

struct StarmapData {
    std::uint32_t consolidated_size{};
    std::vector<StarmapRecord> records;
};

struct GuideRecord {
    double subject_id{};
    std::string message;
    bool removed{};
    bool protected_record{};
    std::array<std::uint8_t, 84> bytes{};
};

struct GuideData {
    std::uint32_t consolidated_size{};
    std::vector<GuideRecord> records;
};

struct GoesDataResult {
    GoesDataStatus status = GoesDataStatus::ok;
    std::string message;
};

GoesDataResult load_starmap(const std::filesystem::path &path, StarmapData &data);
GoesDataResult load_guide(const std::filesystem::path &path, GuideData &data);
GoesDataResult write_starmap(const std::filesystem::path &path, const StarmapData &data);
GoesDataResult write_guide(const std::filesystem::path &path, const GuideData &data);
std::vector<std::size_t> find_starmap_objects(const StarmapData &data, std::string_view key);
std::optional<std::string> find_starmap_name_by_id(const StarmapData &data, double id);
std::vector<std::size_t> guide_records_for(const GuideData &data, double subject_id);
GoesDataResult append_starmap_label(const std::filesystem::path &path, double id,
                                    std::string_view name, GoesObjectKind kind,
                                    std::int16_t ordinal, std::int32_t &byte_offset);
GoesDataResult remove_starmap_label(const std::filesystem::path &path, std::int32_t byte_offset);

GoesDataResult append_guide_record(const std::filesystem::path &path, double subject_id,
                                   std::string_view message);
GoesDataResult replace_guide_record(const std::filesystem::path &path, double subject_id,
                                    std::size_t live_ordinal, std::string_view message);
GoesDataResult delete_guide_records(const std::filesystem::path &path, double subject_id,
                                    std::size_t first, std::size_t last, std::size_t &removed,
                                    std::size_t &protected_records);

} // namespace noctis
