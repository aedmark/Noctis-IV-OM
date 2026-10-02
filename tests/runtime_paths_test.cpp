#include "runtime_paths.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
bool require(bool condition, const char *message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

std::string read(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write(const std::filesystem::path &path, const char *value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << value;
}
}

int main() {
    bool ok = true;
    const noctis::RuntimePathEnvironment linux_environment{
        "/home/tester", "/media/data", "/media/config", {}, {}};
    const auto linux_paths = noctis::resolve_runtime_paths(
        "/opt/noctis", noctis::RuntimePlatform::linux_desktop, linux_environment);
    ok &= require(linux_paths.resource_dir == "/opt/noctis/res", "Linux resource path mismatch");
    ok &= require(linux_paths.seed_data_dir == "/opt/noctis/defaults", "Linux seed path mismatch");
    ok &= require(linux_paths.user_root == "/media/data/noctis-iv-om", "XDG data path mismatch");
    ok &= require(linux_paths.config_dir == "/media/config/noctis-iv-om", "XDG config path mismatch");

    auto fallback_environment = linux_environment;
    fallback_environment.xdg_data_home.clear();
    fallback_environment.xdg_config_home.clear();
    const auto fallback_paths = noctis::resolve_runtime_paths(
        "/opt/noctis", noctis::RuntimePlatform::linux_desktop, fallback_environment);
    ok &= require(fallback_paths.user_root == "/home/tester/.local/share/noctis-iv-om",
                  "Linux HOME data fallback mismatch");
    ok &= require(fallback_paths.config_dir == "/home/tester/.config/noctis-iv-om",
                  "Linux HOME config fallback mismatch");

    const noctis::RuntimePathEnvironment windows_environment{
        {}, {}, {}, "C:/Users/Tester/AppData/Local", "C:/Users/Tester/AppData/Roaming"};
    const auto windows_paths = noctis::resolve_runtime_paths(
        "C:/Games/Noctis", noctis::RuntimePlatform::windows_desktop, windows_environment);
    ok &= require(windows_paths.user_root == "C:/Users/Tester/AppData/Local/Noctis IV OM",
                  "Windows data path mismatch");
    ok &= require(windows_paths.config_dir == "C:/Users/Tester/AppData/Roaming/Noctis IV OM",
                  "Windows config path mismatch");

    const auto test_root = std::filesystem::current_path() / "runtime-paths-test";
    std::error_code ignored;
    std::filesystem::remove_all(test_root, ignored);
    const auto install = test_root / "install";
    const auto old = test_root / "old-portable";
    const auto user = test_root / "user";
    write(install / "defaults/STARMAP.BIN", "seed-map");
    write(install / "defaults/GUIDE.BIN", "seed-guide");
    write(old / "data/current.niv", "save");
    write(old / "data/STARMAP.BIN", "player-map");
    write(old / "gallery/SNAP0001.BMP", "photo");
    write(old / "movies/001/00000001.BMP", "movie");
    write(user / "data/GUIDE.BIN", "keep-guide");
    write(test_root / "outside.txt", "do-not-import");
    std::error_code symlink_error;
    std::filesystem::create_symlink(test_root / "outside.txt", old / "gallery/outside-link", symlink_error);

    const auto migration_paths = noctis::resolve_runtime_paths(
        install, noctis::RuntimePlatform::linux_desktop, linux_environment, user, old);
    const auto first = noctis::prepare_runtime_storage(migration_paths);
    ok &= require(first.ok, first.message.c_str());
    ok &= require(first.copied_files == 4, "migration copied-file count mismatch");
    ok &= require(first.preserved_files == 2, "migration preserved-file count mismatch");
    ok &= require(read(user / "data/current.niv") == "save", "save was not migrated");
    ok &= require(read(user / "data/STARMAP.BIN") == "player-map", "player starmap lost to seed");
    ok &= require(read(user / "data/GUIDE.BIN") == "keep-guide", "destination guide was overwritten");
    ok &= require(read(user / "gallery/SNAP0001.BMP") == "photo", "gallery was not migrated");
    ok &= require(read(user / "movies/001/00000001.BMP") == "movie", "movie was not migrated");
    if (!symlink_error) {
        ok &= require(!std::filesystem::exists(user / "gallery/outside-link"),
                      "migration followed or copied a symbolic link");
    }
    ok &= require(read(old / "data/current.niv") == "save", "migration removed its source");
    ok &= require(std::filesystem::is_directory(user / "config"), "configuration directory missing");

    const auto second = noctis::prepare_runtime_storage(migration_paths);
    ok &= require(second.ok && second.copied_files == 0, "migration was not idempotent");
    ok &= require(read(user / "data/GUIDE.BIN") == "keep-guide", "rerun overwrote destination data");

    const auto clean_user = test_root / "clean-user";
    const auto clean_paths = noctis::resolve_runtime_paths(
        install, noctis::RuntimePlatform::linux_desktop, linux_environment, clean_user,
        test_root / "no-legacy-data");
    const auto clean = noctis::prepare_runtime_storage(clean_paths);
    ok &= require(clean.ok, clean.message.c_str());
    ok &= require(read(clean_user / "data/STARMAP.BIN") == "seed-map", "clean starmap seed missing");
    ok &= require(read(clean_user / "data/GUIDE.BIN") == "seed-guide", "clean guide seed missing");

    const auto broken_paths = noctis::resolve_runtime_paths(
        test_root / "broken-install", noctis::RuntimePlatform::linux_desktop,
        linux_environment, test_root / "broken-user", test_root / "no-legacy-data");
    const auto broken = noctis::prepare_runtime_storage(broken_paths);
    ok &= require(!broken.ok && broken.message.find("required catalog") != std::string::npos,
                  "missing catalog seeds did not stop profile preparation");

    const auto portable_install = test_root / "portable-game";
    write(portable_install / "defaults/STARMAP.BIN", "portable-map");
    write(portable_install / "defaults/GUIDE.BIN", "portable-guide");
    const auto portable_paths = noctis::resolve_runtime_paths(
        portable_install, noctis::RuntimePlatform::linux_desktop, linux_environment, portable_install);
    ok &= require(portable_paths.user_root == portable_install, "portable user_root mismatch");
    ok &= require(portable_paths.data_dir == portable_install / "data", "portable data_dir mismatch");
    ok &= require(portable_paths.config_dir == portable_install / "config", "portable config_dir mismatch");
    const auto portable_prep = noctis::prepare_runtime_storage(portable_paths);
    ok &= require(portable_prep.ok, portable_prep.message.c_str());
    ok &= require(read(portable_install / "data/STARMAP.BIN") == "portable-map", "portable map seed missing");
    ok &= require(read(portable_install / "data/GUIDE.BIN") == "portable-guide", "portable guide seed missing");
    ok &= require(std::filesystem::is_directory(portable_install / "gallery"), "portable gallery missing");
    ok &= require(std::filesystem::is_directory(portable_install / "movies"), "portable movies missing");
    ok &= require(std::filesystem::is_directory(portable_install / "config"), "portable config missing");

    ok &= require(noctis::initialize_runtime_paths(nullptr, std::nullopt, std::nullopt, nullptr, true),
                  "portable mode initialization failed");
    ok &= require(noctis::runtime_paths().user_root == noctis::runtime_paths().executable_dir,
                  "initialized portable user_root mismatch");
    ok &= require(noctis::runtime_paths().data_dir == noctis::runtime_paths().executable_dir / "data",
                  "initialized portable data_dir mismatch");

    ok &= require(noctis::initialize_runtime_paths(nullptr, std::nullopt, std::nullopt, nullptr, false),
                  "system mode initialization failed");
    ok &= require(noctis::runtime_paths().user_root != noctis::runtime_paths().executable_dir,
                  "system user_root should differ from executable_dir");

    std::filesystem::remove_all(test_root, ignored);
    return ok ? 0 : 1;
}
