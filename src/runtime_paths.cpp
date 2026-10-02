#include "runtime_paths.h"

#include <array>
#include <cstdlib>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace noctis {
namespace {

RuntimePaths active_paths;

std::filesystem::path environment_path(const char *name) {
    const char *value = std::getenv(name);
    return value != nullptr && *value != '\0' ? std::filesystem::path(value) : std::filesystem::path{};
}

#if defined(_WIN32)
std::filesystem::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR value = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &value))) return {};
    std::filesystem::path result(value);
    CoTaskMemFree(value);
    return result;
}
#endif

std::filesystem::path executable_directory(const char *argv0) {
#if defined(_WIN32)
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length != 0 && length < buffer.size()) {
        buffer.resize(length);
        return std::filesystem::path(buffer).parent_path();
    }
#elif defined(__linux__)
    std::array<char, 4096> buffer{};
    const auto length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length > 0) {
        buffer[static_cast<std::size_t>(length)] = '\0';
        return std::filesystem::path(buffer.data()).parent_path();
    }
#endif
    std::error_code error;
    auto path = std::filesystem::absolute(argv0 != nullptr ? argv0 : "nivlr", error);
    return error ? std::filesystem::path{} : path.parent_path();
}

bool same_path(const std::filesystem::path &left, const std::filesystem::path &right) {
    std::error_code error;
    const bool equivalent = std::filesystem::equivalent(left, right, error);
    if (!error) return equivalent;
    return left.lexically_normal() == right.lexically_normal();
}

bool copy_file_if_missing(const std::filesystem::path &source,
                          const std::filesystem::path &destination,
                          RuntimeSetupResult &result) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(source, error) || error) return true;
    error.clear();
    if (std::filesystem::exists(destination, error)) {
        if (error) {
            result.message = "could not inspect migration destination " + destination.string() + ": " + error.message();
            return false;
        }
        ++result.preserved_files;
        return true;
    }
    std::filesystem::create_directories(destination.parent_path(), error);
    if (error) {
        result.message = "could not create " + destination.parent_path().string() + ": " + error.message();
        return false;
    }
    if (!std::filesystem::copy_file(source, destination, std::filesystem::copy_options::none, error)) {
        result.message = "could not copy " + source.string() + " to " + destination.string()
            + ": " + error.message();
        return false;
    }
    ++result.copied_files;
    return true;
}

bool copy_tree_if_missing(const std::filesystem::path &source,
                          const std::filesystem::path &destination,
                          RuntimeSetupResult &result) {
    std::error_code error;
    if (!std::filesystem::is_directory(source, error) || error) return true;
    std::filesystem::recursive_directory_iterator iterator(
        source, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end;
    if (error) {
        result.message = "could not inspect migration directory " + source.string() + ": " + error.message();
        return false;
    }
    while (iterator != end) {
        const auto entry = *iterator;
        const auto relative = entry.path().lexically_relative(source);
        if (entry.is_symlink(error)) {
            iterator.disable_recursion_pending();
            error.clear();
        } else if (entry.is_directory(error)) {
            std::filesystem::create_directories(destination / relative, error);
            if (error) {
                result.message = "could not create migration directory "
                    + (destination / relative).string() + ": " + error.message();
                return false;
            }
        } else if (entry.is_regular_file(error)) {
            if (!copy_file_if_missing(entry.path(), destination / relative, result)) return false;
        }
        error.clear();
        iterator.increment(error);
        if (error) {
            result.message = "could not continue migration from " + source.string() + ": " + error.message();
            return false;
        }
    }
    return true;
}

} // namespace

RuntimePaths resolve_runtime_paths(const std::filesystem::path &executable_dir,
                                   RuntimePlatform platform,
                                   const RuntimePathEnvironment &environment,
                                   const std::optional<std::filesystem::path> &user_root_override,
                                   const std::optional<std::filesystem::path> &migration_source_override,
                                   const std::optional<std::filesystem::path> &resource_dir_override) {
    RuntimePaths paths;
    paths.executable_dir = executable_dir;
    paths.resource_dir = resource_dir_override.value_or(executable_dir / "res");
    paths.seed_data_dir = executable_dir / "defaults";
    if (user_root_override) {
        paths.user_root = *user_root_override;
        paths.config_dir = *user_root_override / "config";
    } else if (platform == RuntimePlatform::windows_desktop) {
        paths.user_root = environment.local_app_data / "Noctis IV OM";
        const auto config_base = environment.roaming_app_data.empty()
            ? environment.local_app_data : environment.roaming_app_data;
        paths.config_dir = config_base / "Noctis IV OM";
    } else {
        const auto data_base = environment.xdg_data_home.empty()
            ? environment.home / ".local" / "share" : environment.xdg_data_home;
        const auto config_base = environment.xdg_config_home.empty()
            ? environment.home / ".config" : environment.xdg_config_home;
        paths.user_root = data_base / "noctis-iv-om";
        paths.config_dir = config_base / "noctis-iv-om";
    }
    paths.data_dir = paths.user_root / "data";
    paths.gallery_dir = paths.user_root / "gallery";
    paths.movies_dir = paths.user_root / "movies";
    paths.migration_source = migration_source_override.value_or(executable_dir);
    return paths;
}

bool initialize_runtime_paths(const char *argv0,
                              const std::optional<std::filesystem::path> &user_root_override,
                              const std::optional<std::filesystem::path> &migration_source_override,
                              std::string *error) {
    const auto executable_dir = executable_directory(argv0);
    if (executable_dir.empty()) {
        if (error) *error = "could not determine the executable directory";
        return false;
    }

    RuntimePathEnvironment environment;
    environment.home = environment_path("HOME");
    environment.xdg_data_home = environment_path("XDG_DATA_HOME");
    environment.xdg_config_home = environment_path("XDG_CONFIG_HOME");
#if defined(_WIN32)
    environment.local_app_data = known_folder(FOLDERID_LocalAppData);
    environment.roaming_app_data = known_folder(FOLDERID_RoamingAppData);
    if (environment.local_app_data.empty()) environment.local_app_data = environment_path("LOCALAPPDATA");
    if (environment.roaming_app_data.empty()) environment.roaming_app_data = environment_path("APPDATA");
    constexpr auto platform = RuntimePlatform::windows_desktop;
#else
    constexpr auto platform = RuntimePlatform::linux_desktop;
#endif
    auto effective_user_root = user_root_override;
    if (!effective_user_root) {
        const auto environment_override = environment_path("NOCTIS_IV_OM_HOME");
        if (!environment_override.empty()) effective_user_root = environment_override;
    }
    if (!effective_user_root) {
#if defined(_WIN32)
        if (environment.local_app_data.empty()) {
            if (error) *error = "Windows Local AppData is unavailable; use --user-data-dir";
            return false;
        }
#else
        if (environment.home.empty()
            && (environment.xdg_data_home.empty() || environment.xdg_config_home.empty())) {
            if (error) *error = "HOME/XDG user directories are unavailable; use --user-data-dir";
            return false;
        }
#endif
    }
    auto resource_override = environment_path("NOCTIS_IV_OM_RESOURCE_DIR");
    active_paths = resolve_runtime_paths(
        executable_dir, platform, environment, effective_user_root, migration_source_override,
        resource_override.empty() ? std::nullopt
                                  : std::optional<std::filesystem::path>(resource_override));
    if (active_paths.user_root.empty() || active_paths.config_dir.empty()) {
        if (error) *error = "could not determine the operating-system user-data directories";
        return false;
    }
    return true;
}

const RuntimePaths &runtime_paths() {
    return active_paths;
}

RuntimeSetupResult prepare_runtime_storage(const RuntimePaths &paths) {
    RuntimeSetupResult result;
    std::error_code error;
    for (const auto &directory : {paths.data_dir, paths.gallery_dir, paths.movies_dir, paths.config_dir}) {
        std::filesystem::create_directories(directory, error);
        if (error) {
            result.message = "could not create runtime directory " + directory.string() + ": " + error.message();
            return result;
        }
    }

    const auto legacy_data = paths.migration_source / "data";
    if (!same_path(legacy_data, paths.data_dir)) {
        for (const char *name : {"current.niv", "current.bin", "surface.niv", "surface.bin",
                                 "STARMAP.BIN", "GUIDE.BIN", "guide-export.txt"}) {
            if (!copy_file_if_missing(legacy_data / name, paths.data_dir / name, result)) return result;
        }
    }
    if (!same_path(paths.migration_source / "gallery", paths.gallery_dir)
        && !copy_tree_if_missing(paths.migration_source / "gallery", paths.gallery_dir, result)) return result;
    if (!same_path(paths.migration_source / "movies", paths.movies_dir)
        && !copy_tree_if_missing(paths.migration_source / "movies", paths.movies_dir, result)) return result;

    for (const char *name : {"STARMAP.BIN", "GUIDE.BIN"}) {
        if (!copy_file_if_missing(paths.seed_data_dir / name, paths.data_dir / name, result)) return result;
        error.clear();
        if (!std::filesystem::is_regular_file(paths.data_dir / name, error) || error) {
            result.message = "required catalog is unavailable in the profile and defaults: "
                + std::string(name);
            return result;
        }
    }
    result.ok = true;
    result.message = "runtime storage ready";
    return result;
}

} // namespace noctis
