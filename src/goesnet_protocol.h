#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace noctis {

constexpr std::size_t goes_command_capacity = 83;
constexpr std::size_t goes_result_columns = 21;

enum class GoesCommand : std::uint8_t {
    none,
    clear,
    help,
    parameters,
    parent,
    set_target,
    dependencies,
    catalog,
    add_note,
    replace_note,
    delete_note,
    list_stars,
    print_guide,
    clean,
    inbox,
    outbox,
    unknown,
};

enum class GoesArgumentShape : std::uint8_t {
    none,
    optional_topic,
    object,
    object_with_optional_range,
    optional_range,
    object_and_note,
    object_record_and_note,
};

enum class GoesCommandDisposition : std::uint8_t {
    resident,
    required_native,
    native_export,
    obsolete_legacy_tool,
};

struct GoesCommandDescriptor {
    GoesCommand command;
    std::string_view name;
    GoesArgumentShape arguments;
    GoesCommandDisposition disposition;
    bool mutates_data;
};

enum class GoesParseStatus : std::uint8_t {
    ok,
    empty,
    too_long,
    invalid_character,
    unknown_command,
    missing_argument,
    unexpected_argument,
};

struct GoesRequest {
    GoesParseStatus status = GoesParseStatus::empty;
    GoesCommand command = GoesCommand::none;
    std::string command_name;
    std::string argument;
};

const std::vector<GoesCommandDescriptor> &goes_command_registry();
const GoesCommandDescriptor *find_goes_command(std::string_view name);
GoesRequest parse_goes_command(std::string_view console_line);

enum class GoesResultStatus : std::uint8_t {
    ok,
    usage_error,
    unavailable,
    corrupt_data,
    not_found,
    ambiguous,
    rejected,
    write_failed,
    unsupported,
};

enum class GoesResultAction : std::uint8_t {
    none,
    clear_output,
    set_remote_target,
    set_local_target,
    catalog_changed,
    export_created,
};

struct GoesResult {
    GoesResultStatus status = GoesResultStatus::ok;
    GoesResultAction action = GoesResultAction::none;
    std::string cells;
    struct Target {
        double x{};
        double y{};
        double z{};
        std::int16_t planet_index{-1};
    };
    std::optional<Target> target;
};

std::string format_goes_rows(const std::vector<std::string_view> &rows);

} // namespace noctis
