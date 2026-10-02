#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace noctis {

enum class RuntimePlatform {
    linux_desktop,
    windows_desktop,
};

struct RuntimePathEnvironment {
    std::filesystem::path home;
    std::filesystem::path xdg_data_home;
    std::filesystem::path xdg_config_home;
    std::filesystem::path local_app_data;
    std::filesystem::path roaming_app_data;
};

struct RuntimePaths {
    std::filesystem::path executable_dir;
    std::filesystem::path resource_dir;
    std::filesystem::path seed_data_dir;
    std::filesystem::path user_root;
    std::filesystem::path data_dir;
    std::filesystem::path gallery_dir;
    std::filesystem::path movies_dir;
    std::filesystem::path config_dir;
    std::filesystem::path migration_source;
};

struct RuntimeSetupResult {
    bool ok = false;
    std::size_t copied_files = 0;
    std::size_t preserved_files = 0;
    std::string message;
};

[[nodiscard]] RuntimePaths resolve_runtime_paths(
    const std::filesystem::path &executable_dir,
    RuntimePlatform platform,
    const RuntimePathEnvironment &environment,
    const std::optional<std::filesystem::path> &user_root_override = std::nullopt,
    const std::optional<std::filesystem::path> &migration_source_override = std::nullopt,
    const std::optional<std::filesystem::path> &resource_dir_override = std::nullopt);

[[nodiscard]] bool initialize_runtime_paths(
    const char *argv0,
    const std::optional<std::filesystem::path> &user_root_override = std::nullopt,
    const std::optional<std::filesystem::path> &migration_source_override = std::nullopt,
    std::string *error = nullptr);
[[nodiscard]] const RuntimePaths &runtime_paths();
[[nodiscard]] RuntimeSetupResult prepare_runtime_storage(const RuntimePaths &paths);

} // namespace noctis
