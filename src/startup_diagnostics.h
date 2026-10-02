#pragma once

#include <string_view>

namespace noctis {
// One JSON object per line on stderr. Does not initialize graphics or write files.
void log_event(std::string_view level, std::string_view event, std::string_view detail);
bool startup_report();
}
