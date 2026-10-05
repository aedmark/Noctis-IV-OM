#pragma once

#include "goesnet_data.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace noctis {

enum class StarmapExchangeStatus : std::uint8_t {
    ok,
    no_records_to_export,
    file_not_found,
    io_error,
    format_invalid,
    payload_corrupted,
    checksum_mismatch,
    capacity_exceeded,
    validation_failed,
};

enum class StarmapConflictKind : std::uint8_t {
    none,
    duplicate_record,          // Same ID and same name: benign duplicate, skipped
    id_conflict,               // Same ID, but conflicting name on celestial body
    name_collision,            // Same name, but assigned to a different celestial body
    protected_id_conflict,     // Attempt to overwrite a canonical seed celestial ID
    protected_name_collision,  // Attempt to rename with a canonical seed name
    invalid_id,                // Non-finite, zero, or unphysical ID
    invalid_name,              // Empty, >20 chars, or invalid characters
    invalid_kind_ordinal,      // Invalid object kind or out-of-range ordinal
    internal_batch_conflict,   // Packet itself contains conflicting records
};

struct StarmapValidationIssue {
    StarmapConflictKind kind = StarmapConflictKind::none;
    double id{};
    std::string name;
    std::string message;
};

struct StarmapExchangeRecord {
    double id{};
    std::string name;
    GoesObjectKind kind = GoesObjectKind::star;
    std::int16_t ordinal{};
    double star_x{};
    double star_y{};
    double star_z{};
    std::string notes;
};

struct StarmapImportOptions {
    bool dry_run = false;
    bool allow_name_alias = false;
    bool skip_conflicts = true;
};

struct StarmapImportReport {
    StarmapExchangeStatus status = StarmapExchangeStatus::ok;
    std::size_t records_scanned = 0;
    std::size_t records_imported = 0;
    std::size_t duplicates_skipped = 0;
    std::size_t id_conflicts = 0;
    std::size_t name_collisions = 0;
    std::size_t protected_conflicts = 0;
    std::size_t invalid_records = 0;
    std::size_t guide_notes_imported = 0;
    std::vector<StarmapValidationIssue> issues;
    std::string summary_message;
};

struct StarmapExportReport {
    StarmapExchangeStatus status = StarmapExchangeStatus::ok;
    std::size_t records_exported = 0;
    std::size_t guide_notes_exported = 0;
    std::filesystem::path binary_packet_path;
    std::filesystem::path json_packet_path;
    std::string summary_message;
};

enum class StarmapPacketFormat : std::uint8_t {
    auto_detect,
    binary_nsm,
    json,
    raw_starmap,
};

// Validate individual record syntax in isolation
StarmapValidationIssue validate_starmap_record_syntax(const StarmapExchangeRecord &record);

// Validate a record against existing starmap and pending batch
StarmapValidationIssue validate_starmap_record_semantics(
    const StarmapExchangeRecord &incoming,
    const StarmapData &existing_map,
    const std::vector<StarmapExchangeRecord> &pending_batch,
    bool allow_name_alias = false);

// Export custom records from active starmap to files (.nsm and/or .json)
StarmapExportReport export_starmap_packet(
    const std::filesystem::path &starmap_path,
    const std::filesystem::path &guide_path,
    const std::filesystem::path &out_dir_or_file,
    StarmapPacketFormat format = StarmapPacketFormat::auto_detect,
    std::string_view author_callsign = "STARDRIFTER");

// Import records from packet file into target starmap
StarmapImportReport import_starmap_packet(
    const std::filesystem::path &target_starmap_path,
    const std::filesystem::path &target_guide_path,
    const std::filesystem::path &packet_path,
    const StarmapImportOptions &options = {});

// In-memory packet import / validation
StarmapImportReport import_starmap_packet_bytes(
    StarmapData &target_starmap,
    GuideData *target_guide,
    const std::uint8_t *packet_bytes,
    std::size_t byte_count,
    const StarmapImportOptions &options = {},
    std::string_view filename_hint = "");

// Compact and defragment removed records / tombstones in starmap
GoesDataResult compact_starmap(const std::filesystem::path &starmap_path, std::size_t &records_compacted);

} // namespace noctis
