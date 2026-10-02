#include "startup_diagnostics.h"
#include "runtime_paths.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace noctis {
namespace {
std::string json_string(std::string_view text) {
    constexpr char hex[] = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char ch : text) {
        if (ch == '"' || ch == '\\') {
            out += '\\';
            out += static_cast<char>(ch);
        } else if (ch < 0x20 || ch >= 0x80) {
            out += "\\u00";
            out += hex[ch >> 4];
            out += hex[ch & 15];
        } else {
            out += static_cast<char>(ch);
        }
    }
    return out + '"';
}
}

void log_event(std::string_view level, std::string_view event, std::string_view detail) {
    std::cerr << "{\"level\":" << json_string(level)
              << ",\"event\":" << json_string(event)
              << ",\"detail\":" << json_string(detail) << "}\n";
}

bool startup_report() {
    const auto &paths = runtime_paths();
    log_event("info", "build", "Noctis IV modern port; C++=" + std::to_string(__cplusplus)
        + "; pointer_bits=" + std::to_string(sizeof(void*) * 8));
#if defined(__clang__)
    log_event("info", "compiler", "Clang " __clang_version__);
#elif defined(__GNUC__)
    log_event("info", "compiler", "GCC " __VERSION__);
#elif defined(_MSC_VER)
    log_event("info", "compiler", "MSVC " + std::to_string(_MSC_VER));
#endif
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    log_event(ec ? "error" : "info", "working_directory", ec ? ec.message() : cwd.string());
    bool ready = !ec;
    log_event("info", "executable_directory", paths.executable_dir.string());
    log_event("info", "resource_directory", paths.resource_dir.string());
    log_event("info", "seed_data_directory", paths.seed_data_dir.string());
    log_event("info", "user_data_directory", paths.user_root.string());
    log_event("info", "configuration_directory", paths.config_dir.string());
    log_event("info", "migration_source", paths.migration_source.string());
    // The archive is accessed using fixed offsets from its end. This only checks
    // readability/non-emptiness; it is not a content or compatibility validator.
    const auto archive_path = paths.resource_dir / "supports.nct";
    std::ifstream archive(archive_path, std::ios::binary);
    const bool readable = archive && archive.peek() != std::ifstream::traits_type::eof();
    log_event(readable ? "info" : "error", "resource",
        archive_path.string() + (readable ? ": readable" : ": missing, empty, or unreadable"));
    ready = ready && readable;
    for (const auto &path : {paths.data_dir, paths.gallery_dir, paths.movies_dir, paths.config_dir}) {
        ec.clear();
        bool present = std::filesystem::is_directory(path, ec);
        log_event(present && !ec ? "info" : "warning", "runtime_directory",
            path.string() + (present && !ec ? ": present (writability not tested)"
                                             : ": absent before first normal launch or inaccessible"));
    }
    for (const auto &path : {paths.seed_data_dir / "STARMAP.BIN", paths.seed_data_dir / "GUIDE.BIN",
                             paths.data_dir / "STARMAP.BIN", paths.data_dir / "GUIDE.BIN",
                             paths.data_dir / "current.niv", paths.data_dir / "current.bin",
                             paths.data_dir / "surface.niv", paths.data_dir / "surface.bin"}) {
        std::ifstream input(path, std::ios::binary);
        log_event(input ? "info" : "warning", "optional_data",
            path.string() + (input ? ": readable" : ": absent or unreadable"));
    }
    for (const char *name : {"STARMAP.BIN", "GUIDE.BIN"}) {
        std::ifstream player_catalog(paths.data_dir / name, std::ios::binary);
        std::ifstream seed_catalog(paths.seed_data_dir / name, std::ios::binary);
        const bool available = static_cast<bool>(player_catalog) || static_cast<bool>(seed_catalog);
        log_event(available ? "info" : "error", "required_catalog",
                  std::string(name) + (available ? ": player copy or immutable seed is readable"
                                                 : ": no readable player copy or immutable seed"));
        ready = ready && available;
    }
    log_event(ready ? "info" : "error", "preflight", ready ? "passed; graphics and gameplay not tested" : "failed");
    return ready;
}
}
