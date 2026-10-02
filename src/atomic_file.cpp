#include "atomic_file.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstdio>
#endif

namespace noctis {

bool atomic_replace(const std::filesystem::path &temporary,
                    const std::filesystem::path &destination) {
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(), destination.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
        != 0;
#else
    return std::rename(temporary.c_str(), destination.c_str()) == 0;
#endif
}

} // namespace noctis
