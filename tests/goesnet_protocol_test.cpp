#include "goesnet_protocol.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::fprintf(stderr, "GOESnet protocol: %s\n", message);
    return condition;
}
}

int main() {
    using namespace noctis;
    bool ok = true;

    ok &= require(goes_command_registry().size() == 19, "registry size changed");
    const auto *target = find_goes_command("ST");
    ok &= require(target != nullptr && target->command == GoesCommand::set_target
                      && target->disposition == GoesCommandDisposition::required_native,
                  "ST dispatch metadata mismatch");
    const auto *inbox = find_goes_command("INBOX");
    ok &= require(inbox != nullptr && inbox->disposition == GoesCommandDisposition::obsolete_legacy_tool,
                  "legacy exchange disposition mismatch");
    const auto *log_cmd = find_goes_command("LOG");
    ok &= require(log_cmd != nullptr && log_cmd->command == GoesCommand::flight_log,
                  "LOG command dispatch mismatch");
    const auto *journal_cmd = find_goes_command("JOURNAL");
    ok &= require(journal_cmd != nullptr && journal_cmd->command == GoesCommand::flight_log,
                  "JOURNAL alias mismatch");
    const auto *name_cmd = find_goes_command("NAME");
    ok &= require(name_cmd != nullptr && name_cmd->command == GoesCommand::name_object,
                  "NAME command dispatch mismatch");
    const auto *label_cmd = find_goes_command("LABEL");
    ok &= require(label_cmd != nullptr && label_cmd->command == GoesCommand::name_object,
                  "LABEL alias mismatch");

    const auto par = parse_goes_command(" par new felysia:50000 _");
    ok &= require(par.status == GoesParseStatus::ok && par.command == GoesCommand::parameters
                      && par.argument == "NEW FELYSIA:50000",
                  "PAR normalization or cursor removal mismatch");
    const auto cast = parse_goes_command("CAST FELYSIA:\"GREEN WORLD\"_");
    ok &= require(cast.status == GoesParseStatus::ok && cast.argument == "FELYSIA:'GREEN WORLD'",
                  "historical quote normalization mismatch");
    const auto underscore = parse_goes_command("HELP SET_TARGET_");
    ok &= require(underscore.status == GoesParseStatus::ok && underscore.argument == "SET_TARGET",
                  "entered underscore was mistaken for the cursor");
    const auto clear = parse_goes_command("CLR_");
    ok &= require(clear.status == GoesParseStatus::ok && clear.command == GoesCommand::clear,
                  "resident CLR dispatch mismatch");
    ok &= require(parse_goes_command("_").status == GoesParseStatus::empty, "empty command accepted");
    ok &= require(parse_goes_command("PAR_").status == GoesParseStatus::missing_argument,
                  "missing object accepted");
    ok &= require(parse_goes_command("CLR NOW_").status == GoesParseStatus::unexpected_argument,
                  "unexpected CLR argument accepted");
    ok &= require(parse_goes_command("WARP MIRACLE_").status == GoesParseStatus::unknown_command,
                  "unknown command accepted");
    ok &= require(parse_goes_command("PAR A&B_").status == GoesParseStatus::invalid_character,
                  "historically excluded character accepted");
    ok &= require(parse_goes_command(std::string(84, 'A') + "_").status == GoesParseStatus::too_long,
                  "overlong command accepted");

    const auto cells = format_goes_rows({"SUBJECT: STAR;", "123456789012345678901234", ""});
    ok &= require(cells.size() == 3 * goes_result_columns, "result is not row-aligned");
    ok &= require(cells.substr(0, 21) == "SUBJECT: STAR;       ", "short result row was not padded");
    ok &= require(cells.substr(21, 21) == "123456789012345678901", "long result row was not truncated");
    ok &= require(cells.substr(42, 21) == std::string(21, ' '), "empty result row mismatch");

    return ok ? 0 : 1;
}
