#pragma once

#include <filesystem>

namespace noctis {

// Publish a fully written temporary file over its destination. Both paths
// must be on the same filesystem so the replacement remains one operation.
[[nodiscard]] bool atomic_replace(const std::filesystem::path &temporary,
                                  const std::filesystem::path &destination);

} // namespace noctis
