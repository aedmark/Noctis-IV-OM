#include "plus_presentation.h"

#include <cstdio>

namespace {
bool require(bool value, const char *message) {
    if (!value) std::fprintf(stderr, "plus presentation: %s\n", message);
    return value;
}
}

int main() {
    bool ok = true;
    ok &= require(noctis::format_radius(5.021F) == "RADIUS: 5.0210 CENTIDYAMS",
                  "target radius formatting changed");
    ok &= require(noctis::cycle_lens_flare_mode(0) == 1
                      && noctis::cycle_lens_flare_mode(1) == -1
                      && noctis::cycle_lens_flare_mode(-1) == 0,
                  "lens-flare mode cycle changed");
    ok &= require(noctis::lens_flare_on_hud(true, 0)
                      && !noctis::lens_flare_on_hud(false, 0)
                      && noctis::lens_flare_on_hud(false, 1)
                      && !noctis::lens_flare_on_hud(true, -1),
                  "lens-flare override changed");
    ok &= require(noctis::visible_surface_objects(9, 16) == 9
                      && noctis::visible_surface_objects(9, 255) == 9
                      && noctis::visible_surface_objects(9, 256) == 4,
                  "extended surface viewfield threshold changed");
    const auto &space = noctis::plus_help_lines(false);
    const auto &surface = noctis::plus_help_lines(true);
    ok &= require(space.size() == 4 && surface.size() == 4
                      && space[1].find("RAW SNAPSHOT") != std::string_view::npos
                      && surface[1].find("JETPACK") != std::string_view::npos,
                  "F1 help content is incomplete");
    const auto menu = noctis::plus_visual_menu_lines(false, -1, true);
    ok &= require(menu.size() == 4 && menu[1] == "HUD TEXT OFF (T)"
                      && menu[2] == "LENS FLARES ALWAYS OFF (F)"
                      && menu[3] == "SEAMLESS BORDER (B)",
                  "F2 menu state text changed");
    const auto movie = noctis::plus_movie_menu_lines(7, 3, false, true, false, false, 0.0);
    const auto recording = noctis::plus_movie_menu_lines(7, 3, true, false, true, false, 12.5);
    ok &= require(movie.size() == 5 && movie[1].find("007 EXISTS") != std::string::npos
                      && movie[2].find("003") != std::string::npos
                      && movie[4] == "START RECORDING (ENTER)"
                      && recording[3] == "BLACK FLASH WHEN CAPTURING (F)"
                      && recording[4].find("FPS 12.50") != std::string::npos,
                  "F3 menu state text changed");
    return ok ? 0 : 1;
}
