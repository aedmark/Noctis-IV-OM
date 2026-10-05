#include "starmap_exchange.h"

#include "atomic_file.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <span>
#include <sstream>
#include <string_view>
#include <unordered_set>

namespace noctis {
namespace {

constexpr std::array<std::uint8_t, 8> nsm_magic{'N', 'I', 'V', 'S', 'M', 'A', 'P', '1'};
constexpr std::size_t nsm_header_size = 88;
constexpr std::size_t starmap_record_size = 32;
constexpr std::size_t guide_record_size = 84;
constexpr std::size_t crc_size = 4;
constexpr std::uintmax_t max_packet_size = 64U * 1024U * 1024U;
constexpr double max_celestial_id = 1.0e14;

std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const auto byte : bytes) {
        crc ^= byte;
        for (unsigned bit = 0; bit < 8; ++bit) {
            const auto mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

std::uint32_t read_u32(const std::uint8_t *bytes) {
    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

void write_u32(std::uint8_t *bytes, std::uint32_t value) {
    bytes[0] = static_cast<std::uint8_t>(value);
    bytes[1] = static_cast<std::uint8_t>(value >> 8U);
    bytes[2] = static_cast<std::uint8_t>(value >> 16U);
    bytes[3] = static_cast<std::uint8_t>(value >> 24U);
}

double read_f64(const std::uint8_t *bytes) {
    std::uint64_t bits = 0;
    for (unsigned shift = 0; shift < 64; shift += 8) {
        bits |= static_cast<std::uint64_t>(*bytes++) << shift;
    }
    return std::bit_cast<double>(bits);
}

void write_f64(std::uint8_t *bytes, double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    for (unsigned shift = 0; shift < 64; shift += 8) {
        *bytes++ = static_cast<std::uint8_t>(bits >> shift);
    }
}

std::string_view trim_spaces_view(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n' || s.back() == 0)) {
        s.remove_suffix(1);
    }
    return s;
}

std::string trim_spaces(std::string_view s) {
    return std::string(trim_spaces_view(s));
}

std::string sanitize_name(std::string_view input) {
    std::string s(trim_spaces_view(input));
    for (char &c : s) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - ('a' - 'A'));
        }
    }
    return s;
}

bool is_valid_name_character(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' || c == '-' || c == '\'';
}

std::string escape_json_string(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string unescape_json_string(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            if (next == 'n') out += '\n';
            else if (next == 'r') out += '\r';
            else if (next == 't') out += '\t';
            else out += next;
        } else {
            out += input[i];
        }
    }
    return out;
}

} // namespace

StarmapValidationIssue validate_starmap_record_syntax(const StarmapExchangeRecord &record) {
    StarmapValidationIssue issue;
    issue.id = record.id;
    issue.name = record.name;

    if (!std::isfinite(record.id)) {
        issue.kind = StarmapConflictKind::invalid_id;
        issue.message = "Object ID is non-finite (NaN or Inf)";
        return issue;
    }

    if (std::abs(record.id) < 1.0e-9) {
        issue.kind = StarmapConflictKind::invalid_id;
        issue.message = "Object ID cannot be zero";
        return issue;
    }

    if (std::abs(record.id) > max_celestial_id) {
        issue.kind = StarmapConflictKind::invalid_id;
        issue.message = "Object ID exceeds galactic coordinate bounds";
        return issue;
    }

    const std::string clean_name = sanitize_name(record.name);
    if (clean_name.empty()) {
        issue.kind = StarmapConflictKind::invalid_name;
        issue.message = "Object name is empty or only whitespace";
        return issue;
    }

    if (clean_name.size() > 20) {
        issue.kind = StarmapConflictKind::invalid_name;
        issue.message = "Object name exceeds 20 character maximum limit";
        return issue;
    }

    for (unsigned char c : clean_name) {
        if (!is_valid_name_character(c)) {
            issue.kind = StarmapConflictKind::invalid_name;
            issue.message = "Object name contains invalid character: '" + std::string(1, static_cast<char>(c)) + "'";
            return issue;
        }
    }

    if (record.kind == GoesObjectKind::planet) {
        if (record.ordinal < 1 || record.ordinal > 99) {
            issue.kind = StarmapConflictKind::invalid_kind_ordinal;
            issue.message = "Planet ordinal must be in range 1..99";
            return issue;
        }
        const double star_id = record.id - record.ordinal;
        if (!std::isfinite(star_id) || std::abs(star_id) < 1.0e-9) {
            issue.kind = StarmapConflictKind::invalid_id;
            issue.message = "Calculated parent star ID for planet is invalid";
            return issue;
        }
    } else if (record.kind == GoesObjectKind::star) {
        if (record.ordinal < 0 || record.ordinal > 99) {
            issue.kind = StarmapConflictKind::invalid_kind_ordinal;
            issue.message = "Star class ordinal must be in range 0..99";
            return issue;
        }
    } else {
        issue.kind = StarmapConflictKind::invalid_kind_ordinal;
        issue.message = "Object kind must be star or planet";
        return issue;
    }

    return issue;
}

StarmapValidationIssue validate_starmap_record_semantics(
    const StarmapExchangeRecord &incoming,
    const StarmapData &existing_map,
    const std::vector<StarmapExchangeRecord> &pending_batch,
    bool allow_name_alias) {

    StarmapValidationIssue issue;
    issue.id = incoming.id;
    issue.name = incoming.name;

    const std::string clean_incoming_name = sanitize_name(incoming.name);

    // 1. Check against existing catalog
    for (const auto &existing : existing_map.records) {
        if (existing.removed) continue;

        const bool same_id = std::abs(existing.id - incoming.id) <= goes_id_tolerance;
        const bool same_name = (existing.name == clean_incoming_name);

        if (existing.protected_record) {
            if (same_id) {
                if (same_name && existing.kind == incoming.kind) {
                    issue.kind = StarmapConflictKind::duplicate_record;
                    issue.message = "Record is an exact duplicate of canonical seed object";
                    return issue;
                }
                issue.kind = StarmapConflictKind::protected_id_conflict;
                issue.message = "Cannot rename protected canonical seed object (" + existing.name + ")";
                return issue;
            }
            if (same_name) {
                issue.kind = StarmapConflictKind::protected_name_collision;
                issue.message = "Designation collides with canonical seed name (" + existing.name + ")";
                return issue;
            }
        } else {
            // Local custom record
            if (same_id) {
                if (same_name) {
                    issue.kind = StarmapConflictKind::duplicate_record;
                    issue.message = "Record is already labeled with this exact designation";
                    return issue;
                }
                issue.kind = StarmapConflictKind::id_conflict;
                issue.message = "Celestial body is already named locally as '" + existing.name + "'";
                return issue;
            }
            if (same_name && !allow_name_alias) {
                issue.kind = StarmapConflictKind::name_collision;
                issue.message = "Name '" + clean_incoming_name + "' already assigned to another celestial body locally";
                return issue;
            }
        }
    }

    // 2. Check against pending batch (intra-packet collisions)
    for (const auto &prev : pending_batch) {
        const bool same_id = std::abs(prev.id - incoming.id) <= goes_id_tolerance;
        const bool same_name = (sanitize_name(prev.name) == clean_incoming_name);

        if (same_id) {
            if (same_name) {
                issue.kind = StarmapConflictKind::duplicate_record;
                issue.message = "Duplicate record within import packet";
                return issue;
            }
            issue.kind = StarmapConflictKind::internal_batch_conflict;
            issue.message = "Packet contains multiple conflicting names for body ID " + std::to_string(incoming.id);
            return issue;
        }
        if (same_name && !allow_name_alias) {
            issue.kind = StarmapConflictKind::internal_batch_conflict;
            issue.message = "Packet assigns duplicate name '" + clean_incoming_name + "' to multiple bodies";
            return issue;
        }
    }

    return issue;
}

namespace {

// JSON extraction helper
std::string extract_json_field(std::string_view json, std::string_view key) {
    const std::string pattern = "\"" + std::string(key) + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string_view::npos) return {};
    pos = json.find(':', pos + pattern.size());
    if (pos == std::string_view::npos) return {};
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;
    if (pos >= json.size()) return {};
    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        while (end != std::string_view::npos && json[end - 1] == '\\') {
            end = json.find('"', end + 1);
        }
        if (end == std::string_view::npos) return {};
        return unescape_json_string(json.substr(pos + 1, end - pos - 1));
    }
    // Number, bool, or identifier
    auto end = json.find_first_of(",}\t\r\n ", pos);
    if (end == std::string_view::npos) end = json.size();
    return std::string(json.substr(pos, end - pos));
}

bool parse_json_records(std::string_view json, std::vector<StarmapExchangeRecord> &records,
                        std::vector<std::pair<double, std::string>> &guide_notes) {
    auto rec_pos = json.find("\"records\"");
    if (rec_pos == std::string_view::npos) return false;
    auto arr_start = json.find('[', rec_pos);
    if (arr_start == std::string_view::npos) return false;
    auto arr_end = json.find(']', arr_start);
    if (arr_end == std::string_view::npos) return false;

    std::string_view rec_array = json.substr(arr_start + 1, arr_end - arr_start - 1);

    std::size_t cur = 0;
    while (cur < rec_array.size()) {
        auto obj_start = rec_array.find('{', cur);
        if (obj_start == std::string_view::npos) break;
        auto obj_end = rec_array.find('}', obj_start);
        if (obj_end == std::string_view::npos) break;

        std::string_view obj = rec_array.substr(obj_start, obj_end - obj_start + 1);
        StarmapExchangeRecord r;
        const auto id_str = extract_json_field(obj, "id");
        const auto name_str = extract_json_field(obj, "name");
        const auto kind_str = extract_json_field(obj, "kind");
        const auto ord_str = extract_json_field(obj, "ordinal");
        const auto notes_str = extract_json_field(obj, "notes");

        if (!id_str.empty()) r.id = std::strtod(id_str.c_str(), nullptr);
        r.name = sanitize_name(name_str);
        if (kind_str == "planet" || kind_str == "P") r.kind = GoesObjectKind::planet;
        else r.kind = GoesObjectKind::star;
        if (!ord_str.empty()) r.ordinal = static_cast<std::int16_t>(std::strtol(ord_str.c_str(), nullptr, 10));
        r.notes = notes_str;

        records.push_back(std::move(r));
        cur = obj_end + 1;
    }

    auto notes_pos = json.find("\"guide_notes\"");
    if (notes_pos != std::string_view::npos) {
        auto notes_arr_start = json.find('[', notes_pos);
        if (notes_arr_start != std::string_view::npos) {
            auto notes_arr_end = json.find(']', notes_arr_start);
            if (notes_arr_end != std::string_view::npos) {
                std::string_view notes_array = json.substr(notes_arr_start + 1, notes_arr_end - notes_arr_start - 1);
                std::size_t ncur = 0;
                while (ncur < notes_array.size()) {
                    auto obj_start = notes_array.find('{', ncur);
                    if (obj_start == std::string_view::npos) break;
                    auto obj_end = notes_array.find('}', obj_start);
                    if (obj_end == std::string_view::npos) break;

                    std::string_view nobj = notes_array.substr(obj_start, obj_end - obj_start + 1);
                    const auto sub_id_str = extract_json_field(nobj, "subject_id");
                    const auto msg_str = extract_json_field(nobj, "message");
                    if (!sub_id_str.empty() && !msg_str.empty()) {
                        guide_notes.emplace_back(std::strtod(sub_id_str.c_str(), nullptr), msg_str);
                    }
                    ncur = obj_end + 1;
                }
            }
        }
    }

    return true;
}

} // namespace

StarmapExportReport export_starmap_packet(
    const std::filesystem::path &starmap_path,
    const std::filesystem::path &guide_path,
    const std::filesystem::path &out_dir_or_file,
    StarmapPacketFormat format,
    std::string_view author_callsign) {

    StarmapExportReport report;
    StarmapData starmap;
    auto load_res = load_starmap(starmap_path, starmap);
    if (load_res.status != GoesDataStatus::ok) {
        report.status = StarmapExchangeStatus::io_error;
        report.summary_message = "Could not load starmap from " + starmap_path.string();
        return report;
    }

    GuideData guide;
    load_guide(guide_path, guide);

    // Identify custom player records (offset >= consolidated_size, !removed)
    std::vector<StarmapExchangeRecord> export_records;
    for (const auto &rec : starmap.records) {
        if (!rec.protected_record && !rec.removed && !rec.name.empty()) {
            StarmapExchangeRecord ex;
            ex.id = rec.id;
            ex.name = rec.name;
            ex.kind = rec.kind;
            ex.ordinal = rec.ordinal;
            export_records.push_back(std::move(ex));
        }
    }

    if (export_records.empty()) {
        report.status = StarmapExchangeStatus::no_records_to_export;
        report.summary_message = "No custom starmap records found to export";
        return report;
    }

    // Collect custom guide notes
    std::vector<GuideRecord> export_notes;
    for (const auto &g : guide.records) {
        if (!g.protected_record && !g.removed && !g.message.empty()) {
            export_notes.push_back(g);
        }
    }

    std::filesystem::path target_bin;
    std::filesystem::path target_json;

    std::error_code ec;
    bool is_directory = std::filesystem::is_directory(out_dir_or_file, ec);
    if (is_directory || out_dir_or_file.extension().empty()) {
        std::filesystem::create_directories(out_dir_or_file, ec);
        target_bin = out_dir_or_file / "outbox.nsm";
        target_json = out_dir_or_file / "outbox.json";
    } else {
        if (out_dir_or_file.extension() == ".json") {
            target_json = out_dir_or_file;
            target_bin = out_dir_or_file.parent_path() / (out_dir_or_file.stem().string() + ".nsm");
        } else {
            target_bin = out_dir_or_file;
            target_json = out_dir_or_file.parent_path() / (out_dir_or_file.stem().string() + ".json");
        }
    }

    const std::string author = author_callsign.empty() ? "EXPLORER" : std::string(author_callsign);

    // 1. Build Binary NSM Packet
    if (format == StarmapPacketFormat::auto_detect || format == StarmapPacketFormat::binary_nsm) {
        std::vector<std::uint8_t> payload;
        payload.reserve(nsm_header_size + export_records.size() * starmap_record_size + export_notes.size() * guide_record_size + crc_size);

        // Header: Magic
        payload.insert(payload.end(), nsm_magic.begin(), nsm_magic.end());
        // Version: 1
        payload.resize(payload.size() + 4);
        write_u32(payload.data() + 8, 1);
        // Flags: 0
        payload.resize(payload.size() + 4);
        write_u32(payload.data() + 12, 0);

        // Author (32 bytes)
        std::array<std::uint8_t, 32> author_bytes{};
        std::copy_n(author.data(), std::min(author.size(), std::size_t{31}), author_bytes.begin());
        payload.insert(payload.end(), author_bytes.begin(), author_bytes.end());

        // Timestamp (32 bytes)
        std::array<std::uint8_t, 32> ts_bytes{};
        const std::string ts = "EPOC 6011:556";
        std::copy_n(ts.data(), std::min(ts.size(), std::size_t{31}), ts_bytes.begin());
        payload.insert(payload.end(), ts_bytes.begin(), ts_bytes.end());

        // Starmap record count
        const std::size_t sm_count_offset = payload.size();
        payload.resize(payload.size() + 4);
        write_u32(payload.data() + sm_count_offset, static_cast<std::uint32_t>(export_records.size()));

        // Guide record count
        const std::size_t gd_count_offset = payload.size();
        payload.resize(payload.size() + 4);
        write_u32(payload.data() + gd_count_offset, static_cast<std::uint32_t>(export_notes.size()));

        // Records payload
        for (const auto &ex : export_records) {
            std::array<std::uint8_t, starmap_record_size> rec_bytes{};
            rec_bytes.fill(' ');
            write_f64(rec_bytes.data(), ex.id);
            std::copy_n(ex.name.data(), std::min(ex.name.size(), std::size_t{20}), rec_bytes.begin() + 8);
            rec_bytes[29] = (ex.kind == GoesObjectKind::star ? 'S' : 'P');
            rec_bytes[30] = static_cast<std::uint8_t>('0' + (ex.ordinal / 10));
            rec_bytes[31] = static_cast<std::uint8_t>('0' + (ex.ordinal % 10));
            payload.insert(payload.end(), rec_bytes.begin(), rec_bytes.end());
        }

        // Guide payload
        for (const auto &g : export_notes) {
            std::array<std::uint8_t, guide_record_size> note_bytes{};
            write_f64(note_bytes.data(), g.subject_id);
            std::copy_n(g.message.data(), std::min(g.message.size(), std::size_t{75}), note_bytes.begin() + 8);
            payload.insert(payload.end(), note_bytes.begin(), note_bytes.end());
        }

        // Checksum
        const std::uint32_t checksum = crc32(payload);
        const std::size_t crc_offset = payload.size();
        payload.resize(payload.size() + crc_size);
        write_u32(payload.data() + crc_offset, checksum);

        std::ofstream bin_out(target_bin, std::ios::binary | std::ios::trunc);
        if (bin_out.write(reinterpret_cast<const char *>(payload.data()), static_cast<std::streamsize>(payload.size()))) {
            report.binary_packet_path = target_bin;
        }
    }

    // 2. Build JSON Packet
    if (format == StarmapPacketFormat::auto_detect || format == StarmapPacketFormat::json) {
        std::ostringstream json;
        json << "{\n";
        json << "  \"format\": \"noctis_starmap_exchange\",\n";
        json << "  \"version\": 1,\n";
        json << "  \"author\": \"" << escape_json_string(author) << "\",\n";
        json << "  \"timestamp\": \"EPOC 6011:556\",\n";
        json << "  \"records\": [\n";
        for (std::size_t i = 0; i < export_records.size(); ++i) {
            const auto &r = export_records[i];
            json << "    {\n";
            json << "      \"id\": " << std::setprecision(14) << r.id << ",\n";
            json << "      \"name\": \"" << escape_json_string(r.name) << "\",\n";
            json << "      \"kind\": \"" << (r.kind == GoesObjectKind::star ? "star" : "planet") << "\",\n";
            json << "      \"ordinal\": " << r.ordinal << "\n";
            json << "    }" << (i + 1 < export_records.size() ? "," : "") << "\n";
        }
        json << "  ]";
        if (!export_notes.empty()) {
            json << ",\n  \"guide_notes\": [\n";
            for (std::size_t i = 0; i < export_notes.size(); ++i) {
                const auto &g = export_notes[i];
                json << "    {\n";
                json << "      \"subject_id\": " << std::setprecision(14) << g.subject_id << ",\n";
                json << "      \"message\": \"" << escape_json_string(g.message) << "\"\n";
                json << "    }" << (i + 1 < export_notes.size() ? "," : "") << "\n";
            }
            json << "  ]\n";
        } else {
            json << "\n";
        }
        json << "}\n";

        std::ofstream json_out(target_json, std::ios::trunc);
        if (json_out << json.str()) {
            report.json_packet_path = target_json;
        }
    }

    report.records_exported = export_records.size();
    report.guide_notes_exported = export_notes.size();
    report.status = StarmapExchangeStatus::ok;
    report.summary_message = "Exported " + std::to_string(export_records.size()) + " custom starmap records.";
    return report;
}

StarmapImportReport import_starmap_packet_bytes(
    StarmapData &target_starmap,
    GuideData *target_guide,
    const std::uint8_t *packet_bytes,
    std::size_t byte_count,
    const StarmapImportOptions &options,
    std::string_view filename_hint) {

    StarmapImportReport report;

    if (byte_count == 0 || packet_bytes == nullptr) {
        report.status = StarmapExchangeStatus::payload_corrupted;
        report.summary_message = "Import packet is empty (0 bytes)";
        return report;
    }

    if (byte_count > max_packet_size) {
        report.status = StarmapExchangeStatus::capacity_exceeded;
        report.summary_message = "Packet exceeds maximum size limit (64MB)";
        return report;
    }

    std::vector<StarmapExchangeRecord> incoming_records;
    std::vector<std::pair<double, std::string>> incoming_notes;

    bool is_json = false;
    // Check if JSON
    if (filename_hint.ends_with(".json") || filename_hint.ends_with(".JSON")) {
        is_json = true;
    } else {
        // Skip leading whitespace
        std::size_t lead = 0;
        while (lead < byte_count && (packet_bytes[lead] == ' ' || packet_bytes[lead] == '\t' || packet_bytes[lead] == '\r' || packet_bytes[lead] == '\n')) lead++;
        if (lead < byte_count && packet_bytes[lead] == '{') {
            is_json = true;
        }
    }

    if (is_json) {
        std::string json_text(reinterpret_cast<const char *>(packet_bytes), byte_count);
        if (!parse_json_records(json_text, incoming_records, incoming_notes)) {
            report.status = StarmapExchangeStatus::format_invalid;
            report.summary_message = "Failed to parse JSON packet or missing 'records' array";
            return report;
        }
    } else {
        // Binary packet check: does it start with "NIVSMAP1"?
        const bool has_magic = (byte_count >= 8 && std::equal(nsm_magic.begin(), nsm_magic.end(), packet_bytes));

        if (has_magic) {
            if (byte_count < nsm_header_size + crc_size) {
                report.status = StarmapExchangeStatus::payload_corrupted;
                report.summary_message = "Binary packet truncated before header end";
                return report;
            }

            const std::uint32_t version = read_u32(packet_bytes + 8);
            if (version != 1) {
                report.status = StarmapExchangeStatus::format_invalid;
                report.summary_message = "Unsupported packet version: " + std::to_string(version);
                return report;
            }

            const std::uint32_t starmap_count = read_u32(packet_bytes + 80);
            const std::uint32_t guide_count = read_u32(packet_bytes + 84);

            const std::size_t expected_size = nsm_header_size
                + static_cast<std::size_t>(starmap_count) * starmap_record_size
                + static_cast<std::size_t>(guide_count) * guide_record_size
                + crc_size;

            if (byte_count != expected_size) {
                report.status = StarmapExchangeStatus::payload_corrupted;
                report.summary_message = "Packet size mismatch: expected " + std::to_string(expected_size) + " bytes, got " + std::to_string(byte_count);
                return report;
            }

            // Verify CRC32
            const std::uint32_t stored_crc = read_u32(packet_bytes + (byte_count - crc_size));
            const std::uint32_t computed_crc = crc32(std::span(packet_bytes, byte_count - crc_size));
            if (stored_crc != computed_crc) {
                report.status = StarmapExchangeStatus::checksum_mismatch;
                report.summary_message = "Packet CRC32 checksum mismatch (corruption detected)";
                return report;
            }

            // Read starmap records
            std::size_t offset = nsm_header_size;
            for (std::uint32_t i = 0; i < starmap_count; ++i) {
                StarmapExchangeRecord r;
                r.id = read_f64(packet_bytes + offset);
                r.name = trim_spaces(std::string_view(reinterpret_cast<const char *>(packet_bytes + offset + 8), 20));
                const auto kind_char = packet_bytes[offset + 29];
                r.kind = (kind_char == 'S' ? GoesObjectKind::star : GoesObjectKind::planet);
                const auto d1 = packet_bytes[offset + 30];
                const auto d2 = packet_bytes[offset + 31];
                if (d1 >= '0' && d1 <= '9' && d2 >= '0' && d2 <= '9') {
                    r.ordinal = static_cast<std::int16_t>((d1 - '0') * 10 + (d2 - '0'));
                } else if (d1 == '-' && d2 >= '0' && d2 <= '9') {
                    r.ordinal = static_cast<std::int16_t>(-(d2 - '0'));
                }
                incoming_records.push_back(std::move(r));
                offset += starmap_record_size;
            }

            // Read guide notes
            for (std::uint32_t i = 0; i < guide_count; ++i) {
                const double sub_id = read_f64(packet_bytes + offset);
                const char *msg_start = reinterpret_cast<const char *>(packet_bytes + offset + 8);
                const auto end_ptr = std::find(msg_start, msg_start + 76, '\0');
                std::string msg(msg_start, end_ptr);
                incoming_notes.emplace_back(sub_id, std::move(msg));
                offset += guide_record_size;
            }
        } else {
            // Raw Noctis starmap format: 4-byte header + multiple of 32 bytes
            if (byte_count < 4 || (byte_count - 4) % starmap_record_size != 0) {
                report.status = StarmapExchangeStatus::format_invalid;
                report.summary_message = "File is neither a recognized NSM packet nor record-aligned raw starmap";
                return report;
            }

            const std::uint32_t consolidated_boundary = read_u32(packet_bytes);
            if (consolidated_boundary < 4 || consolidated_boundary > byte_count) {
                report.status = StarmapExchangeStatus::payload_corrupted;
                report.summary_message = "Invalid consolidated boundary in raw starmap";
                return report;
            }

            // Extract records. If consolidated == byte_count, take all non-removed records;
            // otherwise take uncommitted records at >= consolidated_boundary.
            const std::size_t start_offset = (consolidated_boundary == byte_count ? 4 : consolidated_boundary);
            for (std::size_t offset = start_offset; offset < byte_count; offset += starmap_record_size) {
                constexpr std::array<std::uint8_t, 8> removed_marker{'R', 'e', 'm', 'o', 'v', 'e', 'd', ':'};
                if (std::equal(removed_marker.begin(), removed_marker.end(), packet_bytes + offset)) {
                    continue; // Skip tombstones
                }
                StarmapExchangeRecord r;
                r.id = read_f64(packet_bytes + offset);
                r.name = trim_spaces(std::string_view(reinterpret_cast<const char *>(packet_bytes + offset + 8), 20));
                const auto kind_char = packet_bytes[offset + 29];
                r.kind = (kind_char == 'S' ? GoesObjectKind::star : GoesObjectKind::planet);
                const auto d1 = packet_bytes[offset + 30];
                const auto d2 = packet_bytes[offset + 31];
                if (d1 >= '0' && d1 <= '9' && d2 >= '0' && d2 <= '9') {
                    r.ordinal = static_cast<std::int16_t>((d1 - '0') * 10 + (d2 - '0'));
                } else if (d1 == '-' && d2 >= '0' && d2 <= '9') {
                    r.ordinal = static_cast<std::int16_t>(-(d2 - '0'));
                }
                incoming_records.push_back(std::move(r));
            }
        }
    }

    report.records_scanned = incoming_records.size();

    std::vector<StarmapExchangeRecord> accepted_records;
    std::unordered_set<std::size_t> accepted_indices;

    for (std::size_t i = 0; i < incoming_records.size(); ++i) {
        const auto &rec = incoming_records[i];

        // 1. Syntax validation
        const auto syn_issue = validate_starmap_record_syntax(rec);
        if (syn_issue.kind != StarmapConflictKind::none) {
            report.invalid_records++;
            report.issues.push_back(syn_issue);
            continue;
        }

        // 2. Semantic validation
        const auto sem_issue = validate_starmap_record_semantics(
            rec, target_starmap, accepted_records, options.allow_name_alias);

        switch (sem_issue.kind) {
        case StarmapConflictKind::none:
            accepted_records.push_back(rec);
            accepted_indices.insert(i);
            break;
        case StarmapConflictKind::duplicate_record:
            report.duplicates_skipped++;
            report.issues.push_back(sem_issue);
            break;
        case StarmapConflictKind::id_conflict:
            report.id_conflicts++;
            report.issues.push_back(sem_issue);
            break;
        case StarmapConflictKind::name_collision:
            report.name_collisions++;
            report.issues.push_back(sem_issue);
            break;
        case StarmapConflictKind::protected_id_conflict:
        case StarmapConflictKind::protected_name_collision:
            report.protected_conflicts++;
            report.issues.push_back(sem_issue);
            break;
        case StarmapConflictKind::internal_batch_conflict:
            report.id_conflicts++;
            report.issues.push_back(sem_issue);
            break;
        default:
            report.invalid_records++;
            report.issues.push_back(sem_issue);
            break;
        }
    }

    const std::size_t conflict_total = report.id_conflicts + report.name_collisions + report.protected_conflicts + report.invalid_records;

    if (!options.skip_conflicts && conflict_total > 0) {
        report.status = StarmapExchangeStatus::validation_failed;
        report.summary_message = "Import aborted: " + std::to_string(conflict_total) + " conflicts/errors detected.";
        return report;
    }

    report.records_imported = accepted_records.size();

    // If live commit (not dry run)
    if (!options.dry_run && !accepted_records.empty()) {
        for (const auto &acc : accepted_records) {
            StarmapRecord r;
            r.bytes.fill(' ');
            write_f64(r.bytes.data(), acc.id);
            const std::string name = sanitize_name(acc.name);
            std::copy(name.begin(), name.end(), r.bytes.begin() + 8);
            r.bytes[29] = (acc.kind == GoesObjectKind::star ? 'S' : 'P');
            r.bytes[30] = static_cast<std::uint8_t>('0' + (acc.ordinal / 10));
            r.bytes[31] = static_cast<std::uint8_t>('0' + (acc.ordinal % 10));
            r.id = acc.id;
            r.name = name;
            r.kind = acc.kind;
            r.ordinal = acc.ordinal;
            r.removed = false;
            r.protected_record = false;
            target_starmap.records.push_back(std::move(r));
        }

        // Import companion guide notes for accepted or existing bodies
        if (target_guide != nullptr && !incoming_notes.empty()) {
            for (const auto &[subj_id, msg] : incoming_notes) {
                // Check subject exists in starmap
                const bool exists = std::any_of(target_starmap.records.begin(), target_starmap.records.end(),
                    [&](const StarmapRecord &sr) {
                        return !sr.removed && std::abs(sr.id - subj_id) <= goes_id_tolerance;
                    });
                if (!exists) continue;

                // Check note not already present
                const bool duplicate_note = std::any_of(target_guide->records.begin(), target_guide->records.end(),
                    [&](const GuideRecord &gr) {
                        return !gr.removed && std::abs(gr.subject_id - subj_id) <= goes_id_tolerance && gr.message == msg;
                    });
                if (duplicate_note) continue;

                GuideRecord gr;
                gr.bytes.fill(0);
                write_f64(gr.bytes.data(), subj_id);
                std::copy_n(msg.data(), std::min(msg.size(), std::size_t{75}), gr.bytes.begin() + 8);
                gr.subject_id = subj_id;
                gr.message = msg;
                gr.removed = false;
                gr.protected_record = false;
                target_guide->records.push_back(std::move(gr));
                report.guide_notes_imported++;
            }
        }
    }

    report.status = StarmapExchangeStatus::ok;
    report.summary_message = "Processed " + std::to_string(report.records_scanned) + " records: "
        + std::to_string(report.records_imported) + " imported, "
        + std::to_string(report.duplicates_skipped) + " duplicates skipped, "
        + std::to_string(conflict_total) + " conflicts/errors rejected.";

    return report;
}

StarmapImportReport import_starmap_packet(
    const std::filesystem::path &target_starmap_path,
    const std::filesystem::path &target_guide_path,
    const std::filesystem::path &packet_path,
    const StarmapImportOptions &options) {

    StarmapImportReport report;

    std::error_code ec;
    if (!std::filesystem::exists(packet_path, ec) || ec) {
        report.status = StarmapExchangeStatus::file_not_found;
        report.summary_message = "Packet file not found: " + packet_path.string();
        return report;
    }

    std::ifstream input(packet_path, std::ios::binary);
    if (!input) {
        report.status = StarmapExchangeStatus::io_error;
        report.summary_message = "Could not open packet file for reading: " + packet_path.string();
        return report;
    }

    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) {
        report.status = StarmapExchangeStatus::io_error;
        report.summary_message = "I/O error reading packet file: " + packet_path.string();
        return report;
    }

    StarmapData starmap;
    auto load_res = load_starmap(target_starmap_path, starmap);
    if (load_res.status != GoesDataStatus::ok) {
        report.status = StarmapExchangeStatus::io_error;
        report.summary_message = "Could not load target starmap: " + target_starmap_path.string();
        return report;
    }

    GuideData guide;
    GuideData *guide_ptr = nullptr;
    if (!target_guide_path.empty()) {
        if (load_guide(target_guide_path, guide).status == GoesDataStatus::ok) {
            guide_ptr = &guide;
        }
    }

    report = import_starmap_packet_bytes(starmap, guide_ptr, bytes.data(), bytes.size(), options, packet_path.filename().string());

    if (!options.dry_run && report.status == StarmapExchangeStatus::ok && report.records_imported > 0) {
        // Atomic write starmap
        auto write_res = write_starmap(target_starmap_path, starmap);
        if (write_res.status != GoesDataStatus::ok) {
            report.status = StarmapExchangeStatus::io_error;
            report.summary_message = "Failed to write updated starmap: " + write_res.message;
            return report;
        }

        if (guide_ptr != nullptr && report.guide_notes_imported > 0) {
            write_guide(target_guide_path, *guide_ptr);
        }
    }

    return report;
}

GoesDataResult compact_starmap(const std::filesystem::path &starmap_path, std::size_t &records_compacted) {
    records_compacted = 0;
    StarmapData data;
    auto load_res = load_starmap(starmap_path, data);
    if (load_res.status != GoesDataStatus::ok) return load_res;

    std::vector<StarmapRecord> clean_records;
    clean_records.reserve(data.records.size());

    for (auto &rec : data.records) {
        if (rec.protected_record || !rec.removed) {
            clean_records.push_back(std::move(rec));
        } else {
            records_compacted++;
        }
    }

    if (records_compacted == 0) {
        return {}; // Nothing to compact
    }

    data.records = std::move(clean_records);
    return write_starmap(starmap_path, data);
}

} // namespace noctis
