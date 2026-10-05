#include "gallery.h"

#include <cstdlib>
#include <system_error>

#if defined(__EMSCRIPTEN__)
// Emscripten doesn't open host desktop folders.
#elif defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <knownfolders.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace noctis {

std::filesystem::path user_downloads_directory() {
#if defined(__EMSCRIPTEN__)
    return {};
#elif defined(_WIN32)
    PWSTR value = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_DEFAULT, nullptr, &value))) {
        std::filesystem::path result(value);
        CoTaskMemFree(value);
        if (!result.empty()) return result;
    }
    const char *userprofile = std::getenv("USERPROFILE");
    if (userprofile != nullptr && *userprofile != '\0') {
        return std::filesystem::path(userprofile) / "Downloads";
    }
    return {};
#elif defined(__linux__)
    const char *xdg_download = std::getenv("XDG_DOWNLOAD_DIR");
    if (xdg_download != nullptr && *xdg_download != '\0') {
        return std::filesystem::path(xdg_download);
    }
    const char *home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / "Downloads";
    }
    return {};
#else
    const char *home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / "Downloads";
    }
    return {};
#endif
}

bool open_gallery_folder(const std::filesystem::path &path) {
#if defined(__EMSCRIPTEN__)
    (void)path;
    return false;
#elif defined(_WIN32)
    if (path.empty()) return false;
    std::error_code ec;
    std::filesystem::path folder = path;
    if (std::filesystem::is_regular_file(path, ec)) {
        folder = path.parent_path();
    }
    if (!std::filesystem::is_directory(folder, ec)) return false;
    const auto folder_native = folder.wstring();
    HINSTANCE res = ShellExecuteW(nullptr, L"open", folder_native.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(res) > 32;
#elif defined(__linux__)
    if (path.empty()) return false;
    std::error_code ec;
    std::filesystem::path folder = path;
    if (std::filesystem::is_regular_file(path, ec)) {
        folder = path.parent_path();
    }
    if (!std::filesystem::is_directory(folder, ec)) return false;
    pid_t pid = fork();
    if (pid == 0) {
        pid_t grandchild = fork();
        if (grandchild == 0) {
            int devnull = open("/dev/null", O_RDWR);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            execlp("xdg-open", "xdg-open", folder.c_str(), static_cast<char *>(nullptr));
            _exit(127);
        }
        _exit(0);
    }
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
        return true;
    }
    return false;
#else
    (void)path;
    return false;
#endif
}

} // namespace noctis
