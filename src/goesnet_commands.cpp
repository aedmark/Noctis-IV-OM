#include "goesnet_commands.h"

#include "galaxy_sector.h"
#include "goesnet_data.h"
#include "star_properties.h"
#include "system_properties.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace noctis {
namespace {
constexpr std::string_view divider = "&&&&&&&&&&&&&&&&&&&&&";

GoesResult result(GoesResultStatus status, std::vector<std::string> rows,
                  GoesResultAction action = GoesResultAction::none) {
    std::vector<std::string_view> views;
    views.reserve(rows.size());
    for (const auto &row : rows) views.push_back(row);
    return {status, action, format_goes_rows(views), std::nullopt};
}

GoesResult data_failure(const GoesDataResult &failure) {
    switch (failure.status) {
    case GoesDataStatus::not_found: return result(GoesResultStatus::unavailable, {"DATA NOT AVAILABLE"});
    case GoesDataStatus::corrupt: return result(GoesResultStatus::corrupt_data, {"DATA FILE CORRUPT"});
    case GoesDataStatus::rejected: return result(GoesResultStatus::rejected, {"REQUEST REJECTED"});
    case GoesDataStatus::io_error: return result(GoesResultStatus::write_failed, {"DATA ACCESS ERROR"});
    case GoesDataStatus::ok: break;
    }
    return result(GoesResultStatus::corrupt_data, {"DATA ERROR"});
}

std::string display_name(const StarmapRecord &record) {
    return record.name.empty() ? "(UNNAMED)" : record.name;
}

struct ObjectArgument {
    std::string key;
    int range = 100;
    std::string suffix;
    bool valid{};
};

bool parse_positive(std::string_view text, std::size_t &value) {
    if (text.empty()) return false;
    value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9' || value > (std::numeric_limits<std::size_t>::max() - (c - '0')) / 10) return false;
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    return value != 0;
}

ObjectArgument parse_object(std::string_view argument, bool range_suffix) {
    ObjectArgument parsed;
    const auto colon = argument.find(':');
    const auto name = argument.substr(0, colon);
    if (name.empty() || name.size() > 20) return parsed;
    parsed.key.assign(name);
    std::replace(parsed.key.begin(), parsed.key.end(), '_', ' ');
    if (colon != std::string_view::npos) parsed.suffix.assign(argument.substr(colon + 1));
    if (range_suffix && !parsed.suffix.empty()) {
        std::size_t range = 0;
        if (!parse_positive(parsed.suffix, range) || range < 3 || range > 100) return parsed;
        parsed.range = static_cast<int>(range);
    }
    parsed.valid = true;
    return parsed;
}

std::optional<GalaxyStar> resolve_star(double id, double x, double y, double z, int range) {
    const auto start = [range](double coordinate) {
        return static_cast<std::int32_t>((coordinate - range * 50000.0) / 100000.0) * 100000;
    };
    const auto sx = start(x), sy = start(y), sz = start(z);
    for (int ix = 0; ix < range; ++ix) {
        for (int iy = 0; iy < range; ++iy) {
            for (int iz = 0; iz < range; ++iz) {
                const auto star = galaxy_star_at(sx + ix * 100000, sy + iy * 100000, sz + iz * 100000, 0);
                if (!star) continue;
                const double candidate = static_cast<double>(star->x) / 100000 * star->y / 100000 * star->z / 100000;
                if (std::abs(candidate - id) < goes_id_tolerance) return star;
            }
        }
    }
    return std::nullopt;
}

struct ResolvedObject {
    const StarmapRecord *record{};
    GalaxyStar star{};
};

GoesResult lookup_failure(const StarmapData &map, const std::vector<std::size_t> &matches) {
    if (matches.empty()) return result(GoesResultStatus::not_found, {"OBJECT NOT FOUND."});
    std::vector<std::string> rows{"AMBIGUOUS SEARCH KEY:", "PLEASE EXPAND NAME...", std::string(divider), "POSSIBLE RESULTS ARE:", std::string(divider)};
    for (const auto index : matches) rows.push_back(display_name(map.records[index]));
    rows.push_back(std::string(divider));
    return result(GoesResultStatus::ambiguous, std::move(rows));
}

std::optional<ResolvedObject> resolve_object(const StarmapData &map, const ObjectArgument &argument,
                                             const GoesCommandContext &context, GoesResult &failure) {
    const auto matches = find_starmap_objects(map, argument.key);
    if (matches.size() != 1) {
        failure = lookup_failure(map, matches);
        return std::nullopt;
    }
    const auto &record = map.records[matches.front()];
    const double star_id = record.kind == GoesObjectKind::planet ? record.id - record.ordinal : record.id;
    const auto star = resolve_star(star_id, context.observer_x, context.observer_y, context.observer_z, argument.range);
    if (!star) {
        failure = result(GoesResultStatus::not_found, {display_name(record), "IS OUT OF RANGE"});
        return std::nullopt;
    }
    return ResolvedObject{&record, *star};
}

std::string coordinate_row(char axis, double value) {
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%c=%.0f", axis, value);
    return buffer;
}

std::string count_row(std::size_t count) {
    return "    (" + std::to_string(count) + " NOTES)";
}

std::vector<std::string> wrap_note(std::string_view message) {
    std::vector<std::string> rows;
    std::string row;
    std::istringstream words{std::string(message)};
    std::string word;
    while (words >> word) {
        while (word.size() > goes_result_columns) {
            if (!row.empty()) { rows.push_back(row); row.clear(); }
            rows.push_back(word.substr(0, goes_result_columns));
            word.erase(0, goes_result_columns);
        }
        if (row.empty()) row = word;
        else if (row.size() + 1 + word.size() <= goes_result_columns) row += " " + word;
        else { rows.push_back(row); row = word; }
    }
    if (!row.empty()) rows.push_back(row);
    return rows;
}

GoesResult parse_failure(const GoesRequest &request) {
    if (request.status == GoesParseStatus::empty) return result(GoesResultStatus::usage_error, {"TYPE HELP FOR COMMANDS"});
    if (request.status == GoesParseStatus::unknown_command) return result(GoesResultStatus::unsupported, {"(UNKNOWN MODULE)"});
    return result(GoesResultStatus::usage_error, {"INVALID COMMAND SYNTAX", "TYPE HELP FOR COMMANDS"});
}

GoesResult help(std::string_view topic) {
    if (!topic.empty()) {
        const auto *entry = find_goes_command(topic);
        if (!entry) return result(GoesResultStatus::not_found, {"UNKNOWN HELP TOPIC"});
        return result(GoesResultStatus::ok, {std::string(entry->name), "SEE COMMAND REFERENCE"});
    }
    return result(GoesResultStatus::ok, {" GOES COMMAND HELP ", std::string(divider),
        "PAR WHERE ST DL SL", "CAT CAST REP DELE", "PRI CLR HELP", std::string(divider),
        "USE HELP COMMAND"});
}

GoesResult catalog(const StarmapData &map, const GuideData &guide, const ObjectArgument &argument) {
    const auto matches = find_starmap_objects(map, argument.key);
    if (matches.size() != 1) return lookup_failure(map, matches);
    const auto &object = map.records[matches.front()];
    std::size_t first = 1, last = std::numeric_limits<std::size_t>::max();
    if (!argument.suffix.empty()) {
        const auto dots = argument.suffix.find("..");
        if (!parse_positive(argument.suffix.substr(0, dots), first)) return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
        if (dots != std::string::npos && (!parse_positive(argument.suffix.substr(dots + 2), last) || last < first))
            return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
        if (dots == std::string::npos) last = std::numeric_limits<std::size_t>::max();
    }
    std::vector<std::string> rows{" GOES GALACTIC GUIDE ", std::string(divider),
        object.kind == GoesObjectKind::star ? "SUBJECT: STAR;" : "SUBJECT: PLANET;", display_name(object), std::string(divider)};
    const auto records = guide_records_for(guide, object.id);
    for (std::size_t ordinal = first; ordinal <= records.size() && ordinal <= last; ++ordinal) {
        rows.push_back("(" + std::to_string(ordinal) + ")");
        auto wrapped = wrap_note(guide.records[records[ordinal - 1]].message);
        rows.insert(rows.end(), wrapped.begin(), wrapped.end());
    }
    if (records.empty() || first > records.size()) {
        rows.push_back("THERE WERE NO RECORDS");
        rows.push_back("IN THE GUIDE RELATING");
        rows.push_back("SPECIFIED SUBJECT.");
    }
    return result(GoesResultStatus::ok, std::move(rows));
}
} // namespace

GoesResult execute_goes_command(std::string_view console_line, const GoesCommandContext &context) {
    const auto request = parse_goes_command(console_line);
    if (request.status != GoesParseStatus::ok) return parse_failure(request);
    if (request.command == GoesCommand::clear) return {GoesResultStatus::ok, GoesResultAction::clear_output, {}, std::nullopt};
    if (request.command == GoesCommand::help) return help(request.argument);
    if (request.command == GoesCommand::clean || request.command == GoesCommand::inbox || request.command == GoesCommand::outbox)
        return result(GoesResultStatus::unsupported, {"LEGACY TOOL RETIRED", "NATIVE DATA NEEDS NO", "DOS MAINTENANCE"});

    StarmapData map;
    auto loaded = load_starmap(context.starmap_path, map);
    if (loaded.status != GoesDataStatus::ok) return data_failure(loaded);

    if (request.command == GoesCommand::parameters || request.command == GoesCommand::set_target
        || request.command == GoesCommand::dependencies || request.command == GoesCommand::parent) {
        const auto argument = parse_object(request.argument, request.command != GoesCommand::parent);
        if (!argument.valid) return result(GoesResultStatus::usage_error, {"INVALID OBJECT OR RANGE"});
        GoesResult failure;
        const auto object = resolve_object(map, argument, context, failure);
        if (!object) return failure;
        if (request.command == GoesCommand::parameters) {
            return result(GoesResultStatus::ok, {"GOES STARMAP ANALYSIS", std::string(divider),
                object->record->kind == GoesObjectKind::star ? "SUBJECT: STAR;" : "SUBJECT: PLANET;",
                "NAME: " + display_name(*object->record), coordinate_row('X', object->star.x),
                coordinate_row('Y', -object->star.y), coordinate_row('Z', object->star.z)});
        }
        if (request.command == GoesCommand::parent) {
            if (object->record->kind == GoesObjectKind::star)
                return result(GoesResultStatus::rejected, {"THIS OBJECT IS A STAR", "USE THE PAR COMMAND"});
            const double parent_id = object->record->id - object->record->ordinal;
            const auto parent = std::find_if(map.records.begin(), map.records.end(), [&](const auto &candidate) {
                return !candidate.removed && candidate.kind == GoesObjectKind::star
                    && std::abs(candidate.id - parent_id) < goes_id_tolerance;
            });
            if (parent == map.records.end()) return result(GoesResultStatus::not_found, {"PARENT STAR UNKNOWN"});
            return result(GoesResultStatus::ok, {display_name(*object->record), "IS PART OF THE", display_name(*parent), "SYSTEM."});
        }
        if (request.command == GoesCommand::set_target) {
            GoesResult answer;
            if (object->record->kind == GoesObjectKind::star) {
                answer = result(GoesResultStatus::ok, {"REM. TARGET DATA SENT", "STARTING VIMANA DRIVE"}, GoesResultAction::set_remote_target);
                answer.target = GoesResult::Target{static_cast<double>(object->star.x), static_cast<double>(object->star.y), static_cast<double>(object->star.z), -1};
                return answer;
            }
            if (std::abs(context.local_star_x - object->star.x) >= goes_id_tolerance
                || std::abs(context.local_star_y - object->star.y) >= goes_id_tolerance
                || std::abs(context.local_star_z - object->star.z) >= goes_id_tolerance)
                return result(GoesResultStatus::rejected, {"PLANET NOT FOUND AS", "PART OF THIS SYSTEM.", "USE PAR FOR ITS STAR"});
            const auto properties = derive_star_properties(object->star.x, object->star.y, object->star.z);
            const auto system = derive_planet_system(object->star.x, object->star.y, object->star.z,
                                                     properties.star_class, properties.radius);
            const auto index = object->record->ordinal - 1;
            if (index < 0 || index >= system.body_count) return result(GoesResultStatus::not_found, {"PLANET NOT IN SYSTEM"});
            answer = result(GoesResultStatus::ok, {"LOC. TARGET DATA SENT", "BEGIN IN-SYSTEM DRIVE"}, GoesResultAction::set_local_target);
            answer.target = GoesResult::Target{static_cast<double>(object->star.x), static_cast<double>(object->star.y), static_cast<double>(object->star.z), static_cast<std::int16_t>(index)};
            return answer;
        }

        GuideData guide;
        loaded = load_guide(context.guide_path, guide);
        if (loaded.status != GoesDataStatus::ok) return data_failure(loaded);
        const auto properties = derive_star_properties(object->star.x, object->star.y, object->star.z);
        const auto system = derive_planet_system(object->star.x, object->star.y, object->star.z,
                                                 properties.star_class, properties.radius);
        std::vector<std::string> rows{"DEPENDENCIES LISTING:", std::string(divider), "*" + display_name(*object->record)};
        const auto subject_notes = guide_records_for(guide, object->record->id).size();
        if (subject_notes) rows.push_back(count_row(subject_notes));
        for (const auto &candidate : map.records) {
            if (candidate.removed || candidate.kind != GoesObjectKind::planet || candidate.ordinal <= 0
                || std::abs((candidate.id - candidate.ordinal) - (object->record->id - (object->record->kind == GoesObjectKind::planet ? object->record->ordinal : 0))) >= goes_id_tolerance)
                continue;
            const int index = candidate.ordinal - 1;
            if (index < 0 || index >= system.body_count) continue;
            if (object->record->kind == GoesObjectKind::planet && system.bodies[index].owner != object->record->ordinal - 1) continue;
            if (object->record->kind == GoesObjectKind::star && system.bodies[index].owner != -1) continue;
            rows.push_back(display_name(candidate));
            const auto notes = guide_records_for(guide, candidate.id).size();
            if (notes) rows.push_back(count_row(notes));
        }
        rows.push_back(std::string(divider));
        rows.push_back(object->record->kind == GoesObjectKind::star ? "PLANETS LISTING END." : "MOONS LISTING END.");
        return result(GoesResultStatus::ok, std::move(rows));
    }

    const auto argument = parse_object(request.argument, false);
    if ((request.command != GoesCommand::list_stars) && !argument.valid)
        return result(GoesResultStatus::usage_error, {"INVALID COMMAND ARGUMENT"});

    if (request.command == GoesCommand::catalog || request.command == GoesCommand::print_guide) {
        GuideData guide;
        loaded = load_guide(context.guide_path, guide);
        if (loaded.status != GoesDataStatus::ok) return data_failure(loaded);
        auto answer = catalog(map, guide, argument);
        if (request.command == GoesCommand::print_guide && answer.status == GoesResultStatus::ok) {
            std::ofstream output(context.export_path, std::ios::binary | std::ios::trunc);
            if (!output.write(answer.cells.data(), static_cast<std::streamsize>(answer.cells.size())) || !output.flush())
                return result(GoesResultStatus::write_failed, {"EXPORT FAILED"});
            answer.action = GoesResultAction::export_created;
        }
        return answer;
    }

    if (request.command == GoesCommand::add_note || request.command == GoesCommand::replace_note
        || request.command == GoesCommand::delete_note) {
        const auto matches = find_starmap_objects(map, argument.key);
        if (matches.size() != 1) return lookup_failure(map, matches);
        const auto id = map.records[matches.front()].id;
        GoesDataResult mutation;
        if (request.command == GoesCommand::add_note) {
            if (argument.suffix.empty()) return result(GoesResultStatus::usage_error, {"NOTE TEXT REQUIRED"});
            mutation = append_guide_record(context.guide_path, id, argument.suffix);
        } else if (request.command == GoesCommand::replace_note) {
            const auto colon = argument.suffix.find(':');
            std::size_t ordinal = 0;
            if (colon == std::string::npos || !parse_positive(argument.suffix.substr(0, colon), ordinal)
                || colon + 1 == argument.suffix.size()) return result(GoesResultStatus::usage_error, {"RECORD AND NOTE NEEDED"});
            mutation = replace_guide_record(context.guide_path, id, ordinal, argument.suffix.substr(colon + 1));
        } else {
            std::size_t first = 1, last = std::numeric_limits<std::size_t>::max();
            if (!argument.suffix.empty()) {
                const auto dots = argument.suffix.find("..");
                if (!parse_positive(argument.suffix.substr(0, dots), first)) return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
                last = first;
                if (dots != std::string::npos && (!parse_positive(argument.suffix.substr(dots + 2), last) || last < first))
                    return result(GoesResultStatus::usage_error, {"INVALID RECORD RANGE"});
            }
            std::size_t removed = 0, protected_records = 0;
            mutation = delete_guide_records(context.guide_path, id, first, last, removed, protected_records);
            if (mutation.status == GoesDataStatus::ok)
                return result(GoesResultStatus::ok, {"TOTAL SELECTED: " + std::to_string(removed + protected_records),
                    "REMOVED: " + std::to_string(removed), "PROTECTED: " + std::to_string(protected_records)}, GoesResultAction::catalog_changed);
        }
        if (mutation.status != GoesDataStatus::ok) return data_failure(mutation);
        return result(GoesResultStatus::ok, {"MESSAGE ACCEPTED."}, GoesResultAction::catalog_changed);
    }

    if (request.command == GoesCommand::list_stars) {
        std::vector<std::string> rows{request.argument.empty() ? "GLOBAL STARS LISTING:" : "RANGED STARS LISTING:", std::string(divider)};
        if (request.argument.empty()) {
            for (const auto &record : map.records)
                if (!record.removed && record.kind == GoesObjectKind::star && !record.name.empty()) rows.push_back("*" + record.name);
        } else {
            std::size_t requested_range = 0;
            if (!parse_positive(request.argument, requested_range) || requested_range < 3 || requested_range > 100)
                return result(GoesResultStatus::usage_error, {"RANGE MUST BE 3..100"});
            std::unordered_multimap<std::int64_t, std::size_t> ids;
            for (std::size_t index = 0; index < map.records.size(); ++index) {
                const auto &record = map.records[index];
                if (!record.removed && record.kind == GoesObjectKind::star && !record.name.empty())
                    ids.emplace(std::llround(record.id / goes_id_tolerance), index);
            }
            struct Located { std::size_t index; GalaxyStar star; };
            std::vector<Located> located;
            const int range = static_cast<int>(requested_range);
            const auto start = [range](double coordinate) {
                return static_cast<std::int32_t>((coordinate - range * 50000.0) / 100000.0) * 100000;
            };
            const auto sx = start(context.observer_x), sy = start(context.observer_y), sz = start(context.observer_z);
            for (int ix = 0; ix < range; ++ix) for (int iy = 0; iy < range; ++iy) for (int iz = 0; iz < range; ++iz) {
                const auto star = galaxy_star_at(sx + ix * 100000, sy + iy * 100000, sz + iz * 100000, 0);
                if (!star) continue;
                const double id = static_cast<double>(star->x) / 100000 * star->y / 100000 * star->z / 100000;
                const auto key = std::llround(id / goes_id_tolerance);
                for (std::int64_t candidate_key = key - 1; candidate_key <= key + 1; ++candidate_key) {
                    const auto [begin, end] = ids.equal_range(candidate_key);
                    for (auto candidate = begin; candidate != end; ++candidate) {
                        if (std::abs(map.records[candidate->second].id - id) < goes_id_tolerance)
                            located.push_back({candidate->second, *star});
                    }
                }
            }
            std::sort(located.begin(), located.end(), [](const auto &left, const auto &right) { return left.index < right.index; });
            located.erase(std::unique(located.begin(), located.end(), [](const auto &left, const auto &right) { return left.index == right.index; }), located.end());
            for (const auto &entry : located) {
                rows.push_back("*" + map.records[entry.index].name);
                rows.push_back("$" + coordinate_row('X', entry.star.x));
                rows.push_back("$" + coordinate_row('Y', -entry.star.y));
                rows.push_back("$" + coordinate_row('Z', entry.star.z));
                const double dx = context.observer_x - entry.star.x;
                const double dy = context.observer_y - entry.star.y;
                const double dz = context.observer_z - entry.star.z;
                char distance[40];
                std::snprintf(distance, sizeof(distance), "$D=%.2f L.Y.", std::sqrt(dx * dx + dy * dy + dz * dz) * 5E-5);
                rows.emplace_back(distance);
                rows.push_back("[&&&&&&&&&&&&&&&&&&&&");
            }
        }
        rows.push_back(std::string(divider));
        rows.push_back("STARS LISTING END.");
        return result(GoesResultStatus::ok, std::move(rows));
    }
    return result(GoesResultStatus::unsupported, {"COMMAND NOT IMPLEMENTED"});
}

} // namespace noctis
