#pragma once

#include "native_save.h"

#include <filesystem>
#include <span>
#include <string_view>

namespace noctis {

enum class LegacySituationLayout {
    vanilla_245,
    lr_370,
    nivplus_377,
    nivplus_378,
    nivplus_379,
    nivplus_380,
    nivplus_381,
    nivplus_transitional_382,
};

enum class LegacySurfaceLayout {
    dos_40,
    lr_45,
};

struct LegacySituationImport {
    NativeSaveState state;
    LegacySituationLayout layout;
};

struct LegacySurfaceImport {
    SurfaceSaveState state;
    LegacySurfaceLayout layout;
};

struct SurfaceRestore {
    SurfaceSaveState state;
    bool migrated = false;
    LegacySurfaceLayout legacy_layout = LegacySurfaceLayout::dos_40;
};

NativeSaveResult import_legacy_situation(std::span<const std::uint8_t> bytes, LegacySituationImport &result);
NativeSaveResult load_legacy_situation(const std::filesystem::path &path, LegacySituationImport &result);
NativeSaveResult import_legacy_surface(std::span<const std::uint8_t> bytes, LegacySurfaceImport &result);
NativeSaveResult load_legacy_surface(const std::filesystem::path &path, LegacySurfaceImport &result);
NativeSaveResult load_or_migrate_surface(const std::filesystem::path &native_path,
                                         const std::filesystem::path &legacy_path,
                                         SurfaceRestore &result);
std::string_view legacy_layout_name(LegacySituationLayout layout);
std::string_view legacy_layout_name(LegacySurfaceLayout layout);

} // namespace noctis
