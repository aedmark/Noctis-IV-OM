#include "goesnet_protocol.h"

#include <algorithm>
#include <array>

namespace noctis {
namespace {

constexpr std::array<GoesCommandDescriptor, 21> registry{{
    {GoesCommand::clear, "CLR", GoesArgumentShape::none, GoesCommandDisposition::resident, false},
    {GoesCommand::help, "HELP", GoesArgumentShape::optional_topic, GoesCommandDisposition::required_native, false},
    {GoesCommand::parameters, "PAR", GoesArgumentShape::object_with_optional_range, GoesCommandDisposition::required_native, false},
    {GoesCommand::parent, "WHERE", GoesArgumentShape::object, GoesCommandDisposition::required_native, false},
    {GoesCommand::set_target, "ST", GoesArgumentShape::object_with_optional_range, GoesCommandDisposition::required_native, false},
    {GoesCommand::dependencies, "DL", GoesArgumentShape::object_with_optional_range, GoesCommandDisposition::required_native, false},
    {GoesCommand::catalog, "CAT", GoesArgumentShape::object_with_optional_range, GoesCommandDisposition::required_native, false},
    {GoesCommand::add_note, "CAST", GoesArgumentShape::object_and_note, GoesCommandDisposition::required_native, true},
    {GoesCommand::replace_note, "REP", GoesArgumentShape::object_record_and_note, GoesCommandDisposition::required_native, true},
    {GoesCommand::delete_note, "DELE", GoesArgumentShape::object_with_optional_range, GoesCommandDisposition::required_native, true},
    {GoesCommand::list_stars, "SL", GoesArgumentShape::optional_range, GoesCommandDisposition::required_native, false},
    {GoesCommand::print_guide, "PRI", GoesArgumentShape::object_with_optional_range, GoesCommandDisposition::native_export, false},
    {GoesCommand::clean, "CLEAN", GoesArgumentShape::none, GoesCommandDisposition::required_native, true},
    {GoesCommand::inbox, "INBOX", GoesArgumentShape::optional_topic, GoesCommandDisposition::required_native, true},
    {GoesCommand::outbox, "OUTBOX", GoesArgumentShape::optional_topic, GoesCommandDisposition::required_native, false},
    {GoesCommand::gallery, "GALLERY", GoesArgumentShape::none, GoesCommandDisposition::required_native, false},
    {GoesCommand::view_image, "VIEW", GoesArgumentShape::optional_image, GoesCommandDisposition::required_native, false},
    {GoesCommand::flight_log, "LOG", GoesArgumentShape::optional_topic, GoesCommandDisposition::required_native, false},
    {GoesCommand::name_object, "NAME", GoesArgumentShape::object_and_note, GoesCommandDisposition::required_native, true},
    {GoesCommand::bookmarks, "BM", GoesArgumentShape::optional_topic, GoesCommandDisposition::required_native, false},
    {GoesCommand::unknown, "", GoesArgumentShape::none, GoesCommandDisposition::resident, false},
}};

bool requires_argument(GoesArgumentShape shape) {
    switch (shape) {
    case GoesArgumentShape::object:
    case GoesArgumentShape::object_with_optional_range:
    case GoesArgumentShape::object_and_note:
    case GoesArgumentShape::object_record_and_note:
        return true;
    case GoesArgumentShape::none:
    case GoesArgumentShape::optional_topic:
    case GoesArgumentShape::optional_range:
    case GoesArgumentShape::optional_image:
        return false;
    }
    return false;
}

bool valid_console_character(unsigned char character) {
    return ((character >= 32 && character <= 90) || character == '_')
        && character != '$' && character != '&' && character != '<' && character != '>';
}

std::string_view trim_spaces(std::string_view value) {
    while (!value.empty() && value.front() == ' ') value.remove_prefix(1);
    while (!value.empty() && value.back() == ' ') value.remove_suffix(1);
    return value;
}

} // namespace

const std::vector<GoesCommandDescriptor> &goes_command_registry() {
    static const std::vector<GoesCommandDescriptor> commands(registry.begin(), registry.end() - 1);
    return commands;
}

const GoesCommandDescriptor *find_goes_command(std::string_view name) {
    if (name == "JOURNAL") name = "LOG";
    if (name == "LABEL") name = "NAME";
    if (name == "BOOKMARK" || name == "BOOKMARKS" || name == "WAYPOINT" || name == "WAYPOINTS") name = "BM";
    if (name == "EXPORT" || name == "SHARE") name = "OUTBOX";
    if (name == "IMPORT") name = "INBOX";
    const auto found = std::find_if(registry.begin(), registry.end() - 1,
                                    [name](const auto &entry) { return entry.name == name; });
    return found == registry.end() - 1 ? nullptr : &*found;
}

GoesRequest parse_goes_command(std::string_view console_line) {
    GoesRequest request;

    // The live console stores its visible insertion cursor as the final '_'.
    if (!console_line.empty() && console_line.back() == '_') console_line.remove_suffix(1);
    console_line = trim_spaces(console_line);
    if (console_line.empty()) return request;
    if (console_line.size() > goes_command_capacity) {
        request.status = GoesParseStatus::too_long;
        return request;
    }

    std::string normalized(console_line);
    for (char &character : normalized) {
        if (character >= 'a' && character <= 'z') character = static_cast<char>(character - ('a' - 'A'));
        const auto byte = static_cast<unsigned char>(character);
        if (!valid_console_character(byte)) {
            request.status = GoesParseStatus::invalid_character;
            return request;
        }
        if (character == '"') character = '\'';
    }

    const auto separator = normalized.find(' ');
    request.command_name = normalized.substr(0, separator);
    if (separator != std::string::npos) {
        const auto argument = trim_spaces(std::string_view(normalized).substr(separator + 1));
        request.argument.assign(argument);
    }

    const auto *descriptor = find_goes_command(request.command_name);
    if (descriptor == nullptr) {
        request.status = GoesParseStatus::unknown_command;
        request.command = GoesCommand::unknown;
        return request;
    }
    request.command = descriptor->command;
    if (requires_argument(descriptor->arguments) && request.argument.empty()) {
        request.status = GoesParseStatus::missing_argument;
        return request;
    }
    if (descriptor->arguments == GoesArgumentShape::none && !request.argument.empty()) {
        request.status = GoesParseStatus::unexpected_argument;
        return request;
    }
    request.status = GoesParseStatus::ok;
    return request;
}

std::string format_goes_rows(const std::vector<std::string_view> &rows) {
    std::string cells;
    cells.reserve(rows.size() * goes_result_columns);
    for (const auto row : rows) {
        const auto count = std::min(row.size(), goes_result_columns);
        if (count != 0) cells.append(row.data(), count);
        cells.append(goes_result_columns - count, ' ');
    }
    return cells;
}

} // namespace noctis
