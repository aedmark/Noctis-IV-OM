/*
    The main module of Noctis.
    Supervision functions for the base module.
*/

#include "noctis.h"
#include "audio.h"
#include "controls_config.h"
#include "gamepad.h"
#include "brtl.h"
#include "display.h"
#include "gallery_viewer.h"
#include "upscale.h"
#include "flight_log.h"
#include "bookmarks.h"
#include "navigation_hud.h"
#include "goesnet_commands.h"
#include "goesnet_data.h"
#include "starmap_exchange.h"
#include "indexed_framebuffer.h"
#include "input.h"
#include "legacy_numeric.h"
#include "legacy_save.h"
#include "movie_capture.h"
#include "native_save.h"
#include "noctis-0.h"
#include "noctis-d.h"
#include "plus_controls.h"
#include "plus_presentation.h"
#include "runtime_paths.h"
#include "ship_interface.h"
#include "simulation_clock.h"
#include "startup_diagnostics.h"
#include "travel.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>
#include <raylib.h>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
extern "C" {
EMSCRIPTEN_KEEPALIVE
int nivlr_export_starmap(const char *out_path, int format) {
    const auto starmap = noctis::runtime_paths().data_dir / "STARMAP.BIN";
    const auto guide = noctis::runtime_paths().data_dir / "GUIDE.BIN";
    std::filesystem::path target;
    if (out_path && *out_path) {
        target = std::filesystem::path(out_path);
    } else {
        target = noctis::runtime_paths().data_dir / (format == 2 ? "outbox.json" : "outbox.nsm");
    }
    auto fmt = static_cast<noctis::StarmapPacketFormat>(format);
    auto rep = noctis::export_starmap_packet(starmap, guide, target, fmt);
    return rep.status == noctis::StarmapExchangeStatus::ok ? static_cast<int>(rep.records_exported) : -1;
}

EMSCRIPTEN_KEEPALIVE
int nivlr_import_starmap(const char *in_path, int dry_run) {
    const auto starmap = noctis::runtime_paths().data_dir / "STARMAP.BIN";
    const auto guide = noctis::runtime_paths().data_dir / "GUIDE.BIN";
    std::filesystem::path packet;
    if (in_path && *in_path) {
        packet = std::filesystem::path(in_path);
    } else {
        for (const char *cand : {"inbox.nsm", "inbox.json", "inbox.bin", "outbox.nsm", "outbox.json"}) {
            if (std::filesystem::exists(noctis::runtime_paths().data_dir / cand)) {
                packet = noctis::runtime_paths().data_dir / cand;
                break;
            }
        }
    }
    if (packet.empty() || !std::filesystem::exists(packet)) return -1;
    noctis::StarmapImportOptions opts;
    opts.dry_run = (dry_run != 0);
    opts.skip_conflicts = true;
    auto rep = noctis::import_starmap_packet(starmap, guide, packet, opts);
    return rep.status == noctis::StarmapExchangeStatus::ok ? static_cast<int>(rep.records_imported) : -1;
}
}
#endif

const double deg = M_PI / 180;

extern int8_t exitflag;
extern int8_t entryflag;
extern void planetary_main();
extern float tiredness;

// Stuff imported from noctis-0.cpp to lighten it.

int8_t nsnp           = 1; // Nearstar-Not-Prepared
int8_t manual_target  = 0;
int8_t mt_string_char = 0;
int8_t mt_coord       = 0;
int8_t manual_x_string[11];
int8_t manual_y_string[11];
int8_t manual_z_string[11];

static noctis::TravelPhase g_active_travel_phase = noctis::TravelPhase::arrived;
static float g_active_travel_speed               = 0.0f;

// Set the autopilot travel parameters.
void fix_remote_target() {
    status("TGT FIXED", 105);
    noctis::play_goesnet_chime(true);
    const noctis::TravelPosition position{dzat_x, dzat_y, dzat_z};
    const noctis::TravelPosition target{ap_target_x, ap_target_y, ap_target_z};
    const auto guidance          = noctis::begin_travel(position, target);
    ap_target_initial_d          = guidance.initial_distance;
    requested_vimana_coefficient = guidance.requested_coefficient;
    current_vimana_coefficient   = guidance.current_coefficient;
    vimana_reaction_time         = guidance.reaction_time;
    ap_reached                   = 0;

    if (!noctis::remote_target_in_range(target)) {
        status("OUTOFRANGE", 105);
        noctis::play_goesnet_chime(false);
        ap_targetted = 0;
    } else {
        ap_targetted = 1;
    }
}

void fix_local_target() {
    status("TGT FIXED", 105);
    noctis::play_goesnet_chime(true);
    planet_xyz(ip_targetted);
    const auto guidance            = noctis::begin_travel({dzat_x, dzat_y, dzat_z}, {plx, ply, plz});
    ip_target_initial_d            = guidance.initial_distance;
    requested_approach_coefficient = guidance.requested_coefficient;
    current_approach_coefficient   = guidance.current_coefficient;
    reaction_time                  = guidance.reaction_time;
}

/* Lampada alogena (ovvero il laser a diffusione interno alle zattere). */

void alogena() {
    float x[3], y[3], z[3];
    float lon, dlon, dlon_2;
    int16_t pcol;
    dlon = M_PI / 5;

    if (fabs(cam_z) > 1500) {
        dlon *= fabs(cam_z) / 1500;
    }

    dlon_2 = dlon / 2;
    pcol   = 72 + ilightv;

    if (pcol > 100) {
        pcol = 100;
    }

    x[2] = -10;
    y[2] = 10;
    z[2] = -10;

    for (lon = 0; lon < 2 * M_PI - dlon_2;) {
        x[0] = 15 * sin(lon);
        y[0] = 15 * cos(lon);
        z[0] = (y[0] - x[0]) / 2;
        lon += dlon;
        x[1] = 15 * sin(lon);
        y[1] = 15 * cos(lon);
        z[1] = (y[1] - x[1]) / 2;
        poly3d(x, y, z, 3, pcol);
        pcol += 2;
    }

    if (ilightv == 1 && !elight) {
        lens_flares_for(cam_x, cam_y, cam_z, -10, 10, -10, -50000, 2, 1, 0, 1, 1);
    }
}

/* Selection panels for the on-board computer */

void qsel(float *x, float *y, float *z, uint16_t n) {
    setfx(1);
    x[0] -= 10;
    y[0] -= 10;
    x[1] += 10;
    y[1] -= 10;
    x[2] += 10;
    y[2] += 10;
    x[3] -= 10;
    y[3] += 10;
    poly3d(x, y, z, n, 1);
    chgfx(0);
    x[0] += 10;
    y[0] += 10;
    x[1] -= 10;
    y[1] += 10;
    x[2] -= 10;
    y[2] -= 10;
    x[3] += 10;
    y[3] -= 10;
    poly3d(x, y, z, n, 68);
    resetfx();
}

// All reflections on reflective surfaces.

void reflexes() {
    float x[4], y[4], z[4];
    setfx(1);
    lbxf++;
    /*  if (depolarize) goto norefs;

        if (ilight) {
            x[0] = 3200; y[0] = -425; z[0] = -100;
            x[1] = 1000; y[1] = -415; z[1] = -100;
            x[2] = 3200; y[2] = -400; z[2] = -100;
            poly3d (x, y, z, 3, 1);
            x[0] = 3200; y[0] = -400; z[0] = -100;
            x[1] = 3175; y[1] = -425; z[1] = -100;
            x[2] = 3200; y[2] = +300; z[2] = -100;
            poly3d (x, y, z, 3, 5);
        }

        if (pos_z<-1750||beta<-45||beta>45) {
            x[0] = -1200; y[0] = -525; z[0] = -1800;
            x[1] = +1350; y[1] = -525; z[1] = -1800;
            x[2] = +1300; y[2] = -575; z[2] = -1850;
            poly3d (x, y, z, 3, 1);
            x[0] = +1350; y[0] = -525; z[0] = -1800;
            x[1] = +1350; y[1] = -525; z[1] = -4000;
            poly3d (x, y, z, 3, 1);
            x[0] = -1200; y[0] = +450; z[0] = -1800;
            x[1] = +1350; y[1] = +450; z[1] = -1800;
            x[2] = +1300; y[2] = +500; z[2] = -1850;
            poly3d (x, y, z, 3, 1);
            x[0] = +1350; y[0] = +450; z[0] = -1800;
            x[1] = +1350; y[1] = +450; z[1] = -4000;
            poly3d (x, y, z, 3, 1);
        } */
    if (ap_targetting || ip_targetting) {
        goto noevid;
    }

    if (ilight) {
        setfx(1);
    } else {
        setfx(0);
    }

    z[0] = 0;
    z[1] = 0;
    z[2] = 0;
    z[3] = 0;

    if (s_control) {
        x[0] = -66 * 30 - 10;
        y[0] = (float) (50 * (s_control - 3) - 30);
        x[1] = -46 * 30;
        y[1] = (float) (50 * (s_control - 3) - 30);
        x[2] = -46 * 30;
        y[2] = (float) (50 * (s_control - 2) - 25);
        x[3] = -66 * 30 - 10;
        y[3] = (float) (50 * (s_control - 2) - 25);
        qsel(x, y, z, 4);
    }

    if (sys != 4) {
        x[0] = -45.65 * 30;
        y[0] = -125;
        x[1] = -45.45 * 30;
        y[1] = -125;
        x[2] = -45.45 * 30;
        y[2] = 75;
        x[3] = -45.65 * 30;
        y[3] = 75;

        if (stspeed) {
            poly3d(x, y, z, 4, 32);
        } else {
            if (ilight) {
                poly3d(x, y, z, 4, 8);
            } else {
                poly3d(x, y, z, 4, 80);
            }
        }

        if (s_command) {
            x[0] = (float) (27 * 30 * s_command - 72 * 30 + 10);
            y[0] = -130;
            x[1] = x[0] + 26 * 30 + 10;
            y[1] = -130;
            x[2] = x[1];
            y[2] = -75;
            x[3] = x[0];
            y[3] = -75;
            qsel(x, y, z, 4);
        }
    }

noevid:
    lbxf--;
    setfx(0);
}

/* Additional schemes for the computer screen */

void frame(float x, float y, float l, float h, float borderwidth, uint8_t color) {
    // disegna una cornice rettangolare.
    float vx[4], vy[4], vz[4] = {0, 0, 0, 0};
    float x0 = cam_x;
    float y0 = cam_y;
    setfx(4);
    vx[0] = -l - borderwidth;
    vy[0] = -borderwidth;
    vx[1] = +l + borderwidth;
    vy[1] = vy[0];
    vx[2] = +l + borderwidth;
    vy[2] = +borderwidth;
    vx[3] = -l - borderwidth;
    vy[3] = vy[2];
    cam_x = x0 - x;
    cam_y = y0 - y - h;
    poly3d(vx, vy, vz, 4, color);
    cam_y = y0 - y + h;
    poly3d(vx, vy, vz, 4, color);
    vx[0] = -borderwidth;
    vy[0] = -h - borderwidth;
    vx[1] = +borderwidth;
    vy[1] = vy[0];
    vx[2] = +borderwidth;
    vy[2] = +h + borderwidth;
    vx[3] = -borderwidth;
    vy[3] = vy[2];
    cam_y = y0 - y;
    cam_x = x0 - x - l;
    poly3d(vx, vy, vz, 4, color);
    cam_x = x0 - x + l;
    poly3d(vx, vy, vz, 4, color);
    cam_x = x0;
    cam_y = y0;
    resetfx();
}

// Draw star targeting cross.
void pointer_cross_for(double xlight, double ylight, double zlight) {
    double xx, yy, zz, z2, rx, ry, rz;
    xx = xlight - dzat_x;
    yy = ylight - dzat_y;
    zz = zlight - dzat_z;
    rx = xx * opt_pcosbeta + zz * opt_psinbeta;
    z2 = zz * opt_tcosbeta - xx * opt_tsinbeta;
    rz = z2 * opt_tcosalfa + yy * opt_tsinalfa;
    ry = yy * opt_pcosalfa - z2 * opt_psinalfa;

    if (rz > 1) {
        rx /= rz;
        rx += VIEW_X_CENTER;
        ry /= rz;
        ry += VIEW_Y_CENTER - 2 * internal_res_scale;

        const int32_t scale = internal_res_scale;
        if (rx > 10 * scale && ry > 10 * scale && rx < adapted_width - 10 * scale && ry < adapted_height - 10 * scale) {
            uint32_t offset = (adapted_width * ((uint32_t) ry)) + ((uint32_t) rx);

            for (int16_t i = 0; i < 4; i++) {
                int32_t mod1 = (i == 1 || i == 3) ? 1 : -1;
                int32_t mod2 = (i == 1 || i == 2) ? 1 : -1;

                for (int32_t j = 7 * scale; j > 3 * scale; j--) {
                    adapted[offset + mod1 * adapted_width * j + mod2 * j] = 126;
                }
            }
        }
    }
}

// Write a line on the onboard computer screen.
void cline(int16_t line, const char *text) {
    // Multiply the line index by 128 b/c there are 128 characters per line.
    line *= 128;
    // The non-control section starts 20 characters into the line.
    uint16_t start = line + 20;
    strcpy(&ctb[start], text);
    // The current index, used for the other() function.
    point = start + strlen(text);
}

// Write text following what was just written with cline().
void other(const char *text) {
    strcpy(&ctb[point], text);
    // Increment the current character index by the length of what was just
    // added.
    point += strlen(text);
}

// Write the title of a system check (there are 4 in all).
void control(int16_t line, const char *text) {
    // Multiply the line index by 128 b/c there are 128 characters per line.
    uint16_t start = line * 128;
    strcpy(&ctb[start], text);
}

// Writes the title of a command on the main display.
// These are split up into 4 blocks of 27 characters.
void command(int16_t nr, const char *text) {
    uint16_t length = strlen(text);

    if (length > 27) {
        return;
    }

    int16_t index = 20 + 27 * nr;
    strcpy(&ctb[index], text);
}

// Clear the whole on-board computer screen.
void clear_onboard_screen() { memset(ctb, 0, 512); }

/* On-board operating system management group. */

uint8_t reset_signal         = 55;     // Reset signal (=55)
int8_t force_update          = 0;      // Force screen to refresh
int8_t active_screen         = -1;     // Screen currently active
int16_t osscreen_cursor_x[2] = {0, 0}; // Cursor position (x)
int16_t osscreen_cursor_y[2] = {0, 0}; // Cursor position (y)
uint8_t osscreen[2][7 * 21 + 1];       // Array of GOES screens

void mslocate(int16_t screen_id, int16_t cursor_x, int16_t cursor_y) {
    // Rilocazione cursore (multischermo).
    osscreen_cursor_x[screen_id] = cursor_x;
    osscreen_cursor_y[screen_id] = cursor_y;
}

void mswrite(int16_t screen_id, const char *text) {
    // Scrittura caratteri (multischermo).
    int16_t i, j = 0;
    int8_t symbol;

    while ((symbol = text[j]) != 0) {
        if (symbol >= 32 && symbol <= 96) {
            i = 21 * osscreen_cursor_y[screen_id];
            i += osscreen_cursor_x[screen_id];
            osscreen[screen_id][i] = symbol;
            osscreen_cursor_x[screen_id]++;

            if (osscreen_cursor_x[screen_id] >= 21) {
                osscreen_cursor_x[screen_id] = 0;
                osscreen_cursor_y[screen_id]++;
            }
        } else if (symbol == 13) {
            osscreen_cursor_x[screen_id] = 0;
            osscreen_cursor_y[screen_id]++;
        } else if (symbol == 9) {
            osscreen_cursor_x[screen_id] /= (3 * 4);
            osscreen_cursor_x[screen_id]++;
            osscreen_cursor_x[screen_id] *= (3 * 4);

            if (osscreen_cursor_x[screen_id] >= 21) {
                osscreen_cursor_x[screen_id] = 0;
                osscreen_cursor_y[screen_id]++;
            }
        }

        j++;
    }
}

int8_t gnc_pos            = 0;   // Character number in command line.
int32_t goesfile_pos      = 0;   // Position of the GOES output file
char goesnet_command[120] = "_"; // GOES Net Command Line
std::string goes_output_cells;

namespace {

noctis::NativeSaveState preserved_save_state;

noctis::NativeSaveState capture_native_state() {
    noctis::NativeSaveState state;
#define CAPTURE(name) state.name = name
    CAPTURE(nsync);
    CAPTURE(anti_rad);
    CAPTURE(pl_search);
    CAPTURE(field_amplificator);
    CAPTURE(ilight);
    CAPTURE(ilightv);
    CAPTURE(charge);
    CAPTURE(revcontrols);
    CAPTURE(ap_targetting);
    CAPTURE(ap_targetted);
    CAPTURE(ip_targetting);
    CAPTURE(ip_targetted);
    CAPTURE(ip_reaching);
    CAPTURE(ip_reached);
    CAPTURE(ap_target_spin);
    CAPTURE(ap_target_r);
    CAPTURE(ap_target_g);
    CAPTURE(ap_target_b);
    CAPTURE(nearstar_spin);
    CAPTURE(nearstar_r);
    CAPTURE(nearstar_g);
    CAPTURE(nearstar_b);
    CAPTURE(gburst);
    CAPTURE(menusalwayson);
    CAPTURE(depolarize);
    CAPTURE(sys);
    CAPTURE(pwr);
    CAPTURE(dev_page);
    CAPTURE(ap_target_class);
    CAPTURE(f_ray_elapsed);
    CAPTURE(nearstar_class);
    CAPTURE(nearstar_nop);
    CAPTURE(pos_x);
    CAPTURE(pos_y);
    CAPTURE(pos_z);
    CAPTURE(user_alfa);
    CAPTURE(user_beta);
    CAPTURE(navigation_beta);
    CAPTURE(ap_target_ray);
    CAPTURE(nearstar_ray);
    CAPTURE(dzat_x);
    CAPTURE(dzat_y);
    CAPTURE(dzat_z);
    CAPTURE(ap_target_x);
    CAPTURE(ap_target_y);
    CAPTURE(ap_target_z);
    CAPTURE(nearstar_x);
    CAPTURE(nearstar_y);
    CAPTURE(nearstar_z);
    CAPTURE(helptime);
    CAPTURE(ip_target_initial_d);
    CAPTURE(requested_approach_coefficient);
    CAPTURE(current_approach_coefficient);
    CAPTURE(reaction_time);
    CAPTURE(fcs_status_delay);
    CAPTURE(psys);
    CAPTURE(ap_target_initial_d);
    CAPTURE(requested_vimana_coefficient);
    CAPTURE(current_vimana_coefficient);
    CAPTURE(vimana_reaction_time);
    CAPTURE(lithium_collector);
    CAPTURE(autoscreenoff);
    CAPTURE(ap_reached);
    CAPTURE(lifter);
    CAPTURE(secs);
    CAPTURE(data);
    CAPTURE(surlight);
    CAPTURE(gnc_pos);
    CAPTURE(goesfile_pos);
#undef CAPTURE
    std::copy(std::begin(fcs_status), std::end(fcs_status), state.fcs_status.begin());
    std::copy(std::begin(goesnet_command), std::end(goesnet_command), state.goesnet_command.begin());
    state.last_snapshot     = last_snapshot;
    state.option_mouse_look = option_mouse_look;
    state.roof_speed        = roof_speed;
    state.hud_closed        = preserved_save_state.hud_closed;
    state.draw_hud          = draw_hud;
    state.lens_flare_mode   = lens_flare_mode;
    state.seamless_border   = seamless_border;
    return state;
}

void apply_native_state(const noctis::NativeSaveState &state) {
    preserved_save_state = state;
    last_snapshot        = state.last_snapshot;
    option_mouse_look    = state.option_mouse_look;
    roof_speed           = state.roof_speed;
    draw_hud             = state.draw_hud;
    lens_flare_mode      = state.lens_flare_mode;
    seamless_border      = state.seamless_border;
#define APPLY(name) name = state.name
    APPLY(nsync);
    APPLY(anti_rad);
    APPLY(pl_search);
    APPLY(field_amplificator);
    APPLY(ilight);
    APPLY(ilightv);
    APPLY(charge);
    APPLY(revcontrols);
    APPLY(ap_targetting);
    APPLY(ap_targetted);
    APPLY(ip_targetting);
    APPLY(ip_targetted);
    APPLY(ip_reaching);
    APPLY(ip_reached);
    APPLY(ap_target_spin);
    APPLY(ap_target_r);
    APPLY(ap_target_g);
    APPLY(ap_target_b);
    APPLY(nearstar_spin);
    APPLY(nearstar_r);
    APPLY(nearstar_g);
    APPLY(nearstar_b);
    APPLY(gburst);
    APPLY(menusalwayson);
    APPLY(depolarize);
    APPLY(sys);
    APPLY(pwr);
    APPLY(dev_page);
    APPLY(ap_target_class);
    APPLY(f_ray_elapsed);
    APPLY(nearstar_class);
    APPLY(nearstar_nop);
    APPLY(pos_x);
    APPLY(pos_y);
    APPLY(pos_z);
    APPLY(user_alfa);
    APPLY(user_beta);
    APPLY(navigation_beta);
    APPLY(ap_target_ray);
    APPLY(nearstar_ray);
    APPLY(dzat_x);
    APPLY(dzat_y);
    APPLY(dzat_z);
    APPLY(ap_target_x);
    APPLY(ap_target_y);
    APPLY(ap_target_z);
    APPLY(nearstar_x);
    APPLY(nearstar_y);
    APPLY(nearstar_z);
    APPLY(helptime);
    APPLY(ip_target_initial_d);
    APPLY(requested_approach_coefficient);
    APPLY(current_approach_coefficient);
    APPLY(reaction_time);
    APPLY(fcs_status_delay);
    APPLY(psys);
    APPLY(ap_target_initial_d);
    APPLY(requested_vimana_coefficient);
    APPLY(current_vimana_coefficient);
    APPLY(vimana_reaction_time);
    APPLY(lithium_collector);
    APPLY(autoscreenoff);
    APPLY(ap_reached);
    APPLY(lifter);
    APPLY(secs);
    APPLY(data);
    APPLY(surlight);
    APPLY(gnc_pos);
    APPLY(goesfile_pos);
#undef APPLY
    std::copy(state.fcs_status.begin(), state.fcs_status.end(), std::begin(fcs_status));
    std::snprintf(reinterpret_cast<char *>(fcs_status_extended), sizeof(fcs_status_extended), "%s",
                  reinterpret_cast<char *>(fcs_status));
    std::copy(state.goesnet_command.begin(), state.goesnet_command.end(), std::begin(goesnet_command));
}

} // namespace

void sync_internal_resolution_engine(noctis::InternalResolutionMode /*mode*/) {
    QUADWORDS = static_cast<uint32_t>((adapted_width * adapted_height) / 4);
    pqw = QUADWORDS;
    lbxl = lbx;
    ubxl = ubx;
    lbyl = lby;
    ubyl = uby;
    lbxf = static_cast<float>(static_cast<int32_t>(lbx));
    ubxf = static_cast<float>(static_cast<int32_t>(ubx));
    lbyf = static_cast<float>(static_cast<int32_t>(lby));
    ubyf = static_cast<float>(static_cast<int32_t>(uby));
    x_centro_f = VIEW_X_CENTER;
    y_centro_f = VIEW_Y_CENTER;
    dpp = 210.0f * internal_res_scale;
    change_camera_lens();
}

void save_display_settings_current() {
    noctis::set_setting_draw_hud(draw_hud);
    noctis::set_setting_lens_flare_mode(lens_flare_mode);
    noctis::set_setting_seamless_border(seamless_border);
    noctis::save_display_settings(noctis::runtime_paths().config_dir);
}

void freeze() {
    save_display_settings_current();
    noctis::active_flight_log().save_to_file(noctis::runtime_paths().data_dir / "flight_log.json");
    const auto result = noctis::save_native_save(native_situation_file, capture_native_state());
    if (result.status != noctis::NativeSaveStatus::ok) {
        noctis::log_event("error", "native_save", result.message);
    }
}

void persist_browser_storage() {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (Module.nivlrPersist) Module.nivlrPersist();
    });
#endif
}

// Opens the cockpit image viewer (F4 or the GOES VIEW command).
void open_cockpit_gallery(std::string_view key) {
    if (!noctis::open_gallery_viewer(noctis::runtime_paths().gallery_dir, key)) {
        status("NO IMAGES ON FILE", 75);
    }
}

extern double ap_target_previd;
extern double prev_planet_id;
void update_star_label();
void update_planet_label();

// Native GOESnet dispatch. No process, shell, or interchange file is involved.
void run_goesnet_module() {
    const auto &paths = noctis::runtime_paths();
    std::string sname(reinterpret_cast<const char *>(star_label), 20);
    while (!sname.empty() && sname.back() == ' ') sname.pop_back();
    std::string pname(reinterpret_cast<const char *>(planet_label), 20);
    while (!pname.empty() && pname.back() == ' ') pname.pop_back();

    const noctis::GoesCommandContext context{starmap_file,
                                             paths.data_dir / "GUIDE.BIN",
                                             paths.data_dir / "guide-export.txt",
                                             dzat_x,
                                             dzat_y,
                                             dzat_z,
                                             nearstar_x,
                                             nearstar_y,
                                             nearstar_z,
                                             paths.gallery_dir,
                                             paths.config_dir / "bookmarks.ini",
                                             nearstar_identity,
                                             std::move(sname),
                                             nearstar_class,
                                             ip_targetted,
                                             std::move(pname),
                                             false,
                                             0.0,
                                             0.0};
    auto answer       = noctis::execute_goes_command(std::string_view(goesnet_command, gnc_pos + 1), context);
    goes_output_cells = std::move(answer.cells);
    noctis::play_goesnet_chime(answer.status == noctis::GoesResultStatus::ok);

    if (answer.target && answer.action == noctis::GoesResultAction::set_remote_target) {
        ap_target_x   = answer.target->x;
        ap_target_y   = answer.target->y;
        ap_target_z   = answer.target->z;
        ap_targetting = 0;
        extract_ap_target_infos();
        fix_remote_target();
        if (lithium_collector || manual_target) {
            status("CONFLICT", 50);
        } else if (pwr > 15000) {
            stspeed      = 1;
            nsnp         = 1;
            ip_reached   = 0;
            ip_targetted = -1;
        }
    } else if (answer.action == noctis::GoesResultAction::open_image) {
        open_cockpit_gallery(answer.image_id);
    } else if (answer.target && answer.action == noctis::GoesResultAction::set_local_target) {
        if (!ap_reached) {
            status("NEED RECAL", 75);
        } else if (pwr > 15000) {
            ip_targetted = answer.target->planet_index;
            fix_local_target();
            ip_targetting = 0;
            ip_reached    = 0;
            ip_reaching   = 1;
        }
    } else if (answer.action == noctis::GoesResultAction::catalog_changed) {
        ap_target_previd = 12345;
        prev_planet_id   = 12345;
        if (ap_targetted == 0 && (nearstar_x != 0.0 || nearstar_y != 1E8)) {
            ap_target_x   = nearstar_x;
            ap_target_y   = nearstar_y;
            ap_target_z   = nearstar_z;
            ap_targetted  = 1;
            extract_ap_target_infos();
        }
        update_star_label();
        update_planet_label();
        nearstar_labeled++;
    }

    force_update = 1;
    goesfile_pos = 0;
}

/* On-board computer screen tracking group */

uint32_t pp[32] = {0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
                   0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
                   0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
                   0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000};

void digit_at(int8_t digit, float x, float y, float size, uint8_t color, int8_t shader) {
    // This is an alphanumeric character.
    uint8_t *prev_txtr = txtr;
    float vx[4], vy[4], vz[4] = {0, 0, 0, 0};
    float size_x_left  = size * -1.5f;
    float size_y_left  = size * -2.0f;
    float size_x_right = size * +4.0f;
    float size_y_right = size * +8.0f;
    int32_t prev_xs    = XSIZE;
    int32_t prev_ys    = YSIZE;
    int16_t n, m, d, i;
    int8_t pixel_color = color % 64;
    int8_t map_base    = ((uint8_t) (color >> 6u)) << 6u;

    if (reset_signal > 100) {
        pixel_color -= (reset_signal - 100);

        if (pixel_color < 0) {
            pixel_color = 0;
        }
    }

    if (digit > 32 && digit <= 96) {
        txtr = p_surfacemap;
        d    = (digit - 32) * 36;

        for (n = 0; n < 36; n++) {
            i = 256 * n - 5;
            if (i > 0) {
                txtr[i - 1] = 0; // Avoid aliasing at the end of the scanline.
            }

            for (m = 0; m < 32; m++) {
                if (i >= 0) {
                    if (read_u32_le(digimap2 + 4 * (n + d)) & pp[m]) {
                        txtr[i] = pixel_color;
                    } else {
                        txtr[i] = 0;
                    }
                }

                i++;
            }

            if (shader) {
                pixel_color--;
            }
        }

        txtr[256 * 36 - 6] = 0; // Avoids aliasing at the end of the matrix.
        vx[3]              = x + size_x_left;
        vx[0]              = x + size_x_right;
        vx[1]              = x + size_x_right;
        vx[2]              = x + size_x_left;
        vy[3]              = y + size_y_left;
        vy[0]              = y + size_y_left;
        vy[1]              = y + size_y_right;
        vy[2]              = y + size_y_right;
        setfx(2);
        XSIZE = 512;
        YSIZE = 576;
        polymap(vx, vy, vz, 4, map_base);
        XSIZE = prev_xs;
        YSIZE = prev_ys;
        txtr  = prev_txtr;
        resetfx();
    }
}

void screen() {
    float x, y;
    int16_t c, p, t = 0;

    if (ap_targetting) {
        return;
    }

    if (pwr <= 15000 && !charge) {
        fast_srand(legacy_u32_from_double(secs));
        c = fast_random(3) + 2;

        if (simulation_ticks() % c) {
            return;
        }
    }

    x = cam_x;
    y = cam_y;

    for (p = -2; p < 2; p++)
        for (c = -64; c < 64; c++) {
            cam_x = x - ((float) (c * 30));

            if (c < -44) {
                if (s_control || menusalwayson) {
                    cam_x += 50;
                    cam_y = y - ((float) (p * 50));
                } else {
                    t += 19;
                    c += 19;
                    goto passby;
                }
            }

            if (c == -44) {
                if (s_command || menusalwayson) {
                    if (p == -2 || menusalwayson) {
                        cam_y = y - ((float) (p * 50));
                    } else {
                        t += 108;
                        break;
                    }
                } else {
                    if (!infoarea || p == -2) {
                        t += 108;
                        break;
                    } else {
                        cam_y = y - ((float) (p * 46)) - 12;
                    }
                }
            }

            if (ctb[t] >= 'A' && ctb[t] <= 'Z') {
                digit_at(ctb[t], -6, -16, 4, noctis::onboard_text_color(ctb[t]), 1);
            } else {
                if (ctb[t] >= 'a' && ctb[t] <= 'z') {
                    digit_at(ctb[t] - 32, -6, -16, 4, noctis::onboard_text_color(ctb[t]), 1);
                } else {
                    digit_at(ctb[t], -6, -16, 4, noctis::onboard_text_color(ctb[t]), 1);
                }
            }

        passby:
            t++;
        }

    cam_x = x;
    cam_y = y;
}

// Draw the surface map at the time you want to land.

void show_planetary_map() {
    int8_t is_moon;
    int16_t lat, lon, i, j, p;

    if (nearstar_p_owner[ip_targetted] > -1) {
        is_moon = 1;
    } else {
        is_moon = 0;
    }

    lon = landing_pt_lon - 22;

    for (i = 0; i < 48; i++) {
        while (lon > 359) {
            lon -= 360;
        }

        while (lon < 0) {
            lon += 360;
        }

        lat = landing_pt_lat - 14;

        for (j = 0; j < 32; j++) {
            p = ((uint16_t) (j + 9) << 8u) + i + 14;

            if (lat > 0 && lat < 120) {
                ptr = lat * 360 + lon;

                if (is_moon) {
                    p_surfacemap[p] = s_background[ptr] + 128;
                } else {
                    p_surfacemap[p] = p_background[ptr] + 192;
                }
            } else {
                p_surfacemap[p] = 4;
            }

            lat++;
        }

        lon++;
    }
}

/*  Draw the stardrifter. (The one you are using, seen from the inside.)
    It also assumes the task of deciphering keyboard commands for GOESnet. */

int16_t goesk_a = -1;
int16_t goesk_e = -1;

void vehicle(float opencapcount) {
    int16_t n, c, i, j, k;
    int8_t short_text[11];
    uint8_t chcol;
    float backup_cam_x, backup_cam_z;
    float backup_beta = beta;
    float chry;
    float vx[4], vy[4], vz[4];
    float osscreen_x[4], osscreen_z[4];
    float osscreen_y[4] = {-20 * 15, 14 * 15, 14 * 15, -20 * 15};

    if (elight) {
        memset(osscreen[0], 0, 7 * 21);
        memset(osscreen[1], 0, 7 * 21);
    }

    // Tracking of panoramic domes when not closing to form capsule.
    if (opencapcount == 0.0) {
        cam_z += 3100;
        cam_y -= 550;
        polycupola(0, 0);
        setfx(0);
        cupola(0, 8);
        resetfx();
        cam_y += 550;

        if (!ontheroof) {
            polycupola(+1, 0);
            setfx(0);
            cupola(+1, 8);
            resetfx();
        }

        cam_z -= 3100;
    }

    /* Rest of the hull. If you are on the terrace, stop here after drawing the
     * upper dome, which must be superimposed on the hull.
     */
    if (depolarize) {
        setfx(2);
        drawpv(vehicle_handle, 2, 3, 0.0, 0.0, 0.0, 0);
        resetfx();
    } else {
        drawpv(vehicle_handle, 2, 2, 0.0, 0.0, 0.0, 0);
    }

    if (ontheroof) {
        cam_z += 3100;
        polycupola(+1, 0);
        setfx(0);
        cupola(+1, 8);
        resetfx();
        cam_z -= 3100;
        return;
    }

    // Key interception (priority) for GOESnet.

    if (force_update || (active_screen == 0 && is_key())) {
        if (!force_update) {
            goesk_a = -1;
            c       = get_key();

            if (!c) {
                c       = get_key();
                goesk_e = c;

                if (c == 0x47) {
                    goesnet_command[0] = '_';
                    goesnet_command[1] = 0;
                    gnc_pos            = 0;
                    goesk_e            = -1;
                    noctis::play_terminal_keystroke();
                }
            } else {
                if (c == 27) {
                    goesk_a = c; // Pass the keystroke
                } else if (c == 8 && gnc_pos > 0) {
                    goesnet_command[gnc_pos - 1] = '_';
                    goesnet_command[gnc_pos]     = 0;
                    gnc_pos--;
                    noctis::play_terminal_keystroke();
                } else if (c == 13) {
                    noctis::play_goesnet_transmit();
                    run_goesnet_module();
                    n = 0;

                    if (!memcmp(goesnet_command, "CAST", 4)) {
                        i = 0;

                        while (i < gnc_pos) {
                            if (goesnet_command[i] == ':') {
                                n = i + 1;
                                break;
                            }

                            i++;
                        }
                    }
                    goesnet_command[n]     = '_';
                    goesnet_command[n + 1] = 0;
                    gnc_pos                = n;
                }

                // Transform quotation marks.
                if (c == 34) {
                    c = 39;
                }

                // Uppercase letters.
                if (c >= 'a' && c <= 'z') {
                    c -= 32;
                }

                // Check for invalid characters.
                if (c != 36 && c != 38 && c != 60 && c != 62) {
                    // Enter valid characters.
                    if ((c >= 32 && c <= 90 && gnc_pos < 83) || (c == 95)) {
                        goesnet_command[gnc_pos]     = c;
                        goesnet_command[gnc_pos + 1] = '_';
                        goesnet_command[gnc_pos + 2] = 0;
                        gnc_pos++;
                        noctis::play_terminal_keystroke();
                    }
                }
            }
        }

        memset(osscreen[0] + 3 * 21, 0, 4 * 21);
        mslocate(0, 0, 3);
        mswrite(0, (char *) goesnet_command);
    }

    /* Key interception (priority) for the "STARMAP TREE". */

    if (force_update || (active_screen == 1 && is_key())) {
        if (!force_update) {
        krep1:
            c       = get_key();
            goesk_a = c;

            if (!c) {
                goesk_a = -1;
                c       = get_key();
                goesk_e = c;

                switch (c) {
                case 0x4F:
                case 0x76:
                case 0x91: {
                    goesfile_pos =
                        noctis::goes_scroll_offset(goesfile_pos, goes_output_cells.size(), noctis::GoesScroll::end);
                    goesk_e = -1;
                    noctis::play_terminal_scroll();
                    break;
                }
                case 0x47:
                case 0x84:
                case 0x8D:
                    goesfile_pos = noctis::goes_scroll_offset(goesfile_pos, 0, noctis::GoesScroll::home);
                    goesk_e      = -1;
                    noctis::play_terminal_scroll();
                    break;

                case 80:
                    goesfile_pos += noctis::goes_screen_columns;
                    goesk_e = -1;
                    noctis::play_terminal_scroll();
                    break;

                case 72:
                    goesfile_pos -= noctis::goes_screen_columns;

                    if (goesfile_pos < 0) {
                        goesfile_pos = 0;
                    }

                    goesk_e = -1;
                    noctis::play_terminal_scroll();
                    break;

                case 0x51:
                    goesfile_pos += noctis::goes_screen_bytes;
                    goesk_e = -1;
                    noctis::play_terminal_scroll();
                    break;

                case 0x49:
                    goesfile_pos -= noctis::goes_screen_bytes;

                    if (goesfile_pos < 0) {
                        goesfile_pos = 0;
                    }

                    goesk_e = -1;
                    noctis::play_terminal_scroll();
                    break;
                default:
                    break;
                }
            }

            if (is_key()) {
                goto krep1;
            }
        }

        std::array<std::uint8_t, noctis::goes_screen_bytes + 1> page{};
        goesfile_pos = noctis::goes_scroll_offset(goesfile_pos, goes_output_cells.size(), noctis::GoesScroll::none);
        const auto available = goesfile_pos < static_cast<std::int32_t>(goes_output_cells.size())
                                   ? goes_output_cells.size() - static_cast<std::size_t>(goesfile_pos)
                                   : 0;
        const auto count     = std::min<std::size_t>(available, noctis::goes_screen_bytes);
        std::copy_n(goes_output_cells.begin() + goesfile_pos, count, page.begin());
        std::copy(page.begin(), page.end(), std::begin(osscreen[1]));
    }

    // Intercettazione tasti (prioritaria) per la planetary map.

    if (active_screen == 2 && is_key()) {
    krep2:
        c       = get_key();
        goesk_a = c;

        if (!c) {
            goesk_a = -1;
            c       = get_key();
            goesk_e = c;

            if (landing_point) {
                switch (c) {
                case 77:
                    landing_pt_lon++;

                    if (landing_pt_lon >= 360) {
                        landing_pt_lon -= 360;
                    }

                    goesk_e = -1;
                    break;

                case 75:
                    landing_pt_lon--;

                    if (landing_pt_lon < 0) {
                        landing_pt_lon += 360;
                    }

                    goesk_e = -1;
                    break;

                case 80:
                    landing_pt_lat++;

                    if (landing_pt_lat > 119) {
                        landing_pt_lat = 119;
                    }

                    goesk_e = -1;
                    break;

                case 72:
                    landing_pt_lat--;

                    if (landing_pt_lat < 1) {
                        landing_pt_lat = 1;
                    }

                    goesk_e = -1;
                    break;

                case 0x74:
                    landing_pt_lon += 3;

                    if (landing_pt_lon >= 360) {
                        landing_pt_lon -= 360;
                    }

                    goesk_e = -1;
                    break;

                case 0x73:
                    landing_pt_lon -= 3;

                    if (landing_pt_lon < 0) {
                        landing_pt_lon += 360;
                    }

                    goesk_e = -1;
                    break;

                case 0x91:
                    landing_pt_lat += 3;

                    if (landing_pt_lat > 119) {
                        landing_pt_lat = 119;
                    }

                    goesk_e = -1;
                    break;

                case 0x8D:
                    landing_pt_lat -= 3;

                    if (landing_pt_lat < 1) {
                        landing_pt_lat = 1;
                    }

                    goesk_e = -1;
                default:
                    break;
                }
                if (goesk_e == -1) {
                    noctis::play_terminal_scroll();
                }
            }
        } else {
            if (landing_point) {
                if (c == 13) {
                    land_now = 1;
                    goesk_a  = -1;
                    noctis::play_goesnet_chime(true);
                }

                if (c == 27) {
                    landing_point = 0;
                    status("CANCELLED", 50);
                    goesk_a = -1;
                    noctis::play_goesnet_chime(false);
                }

                /*  Unit� di debugging dell'albedo
                    uint8_t far *ov=(uint8_t far*)objectschart;
                    if (c == 'b') {
                    ov[(18 + 60*360) / 2] += 4;
                    p_background[18 + 60*360] ++;
                    }
                    if (c == 'd') {
                    ov[(18 + 60*360) / 2] -= 4;
                    p_background[18 + 60*360] --;
                    }*/
            }
        }

        if (is_key()) {
            goto krep2;
        }
    }

    // Tracciamento degli schermi di GOESNet.
    // Si tratta dei primi due schermi sulla paratia destra.
    H_MATRIXS = 6;
    V_MATRIXS = 3;
    change_txm_repeating_mode();
    txtr          = p_surfacemap + 256 * 8 + 16;
    osscreen_z[0] = -104 * 15;
    osscreen_z[1] = -104 * 15;
    osscreen_z[2] = -154 * 15;
    osscreen_z[3] = -154 * 15;
    osscreen_x[0] = +236 * 15;
    osscreen_x[1] = +236 * 15;
    osscreen_x[2] = +236 * 15;
    osscreen_x[3] = +236 * 15;
    vx[0]         = 236 * 15;
    vx[1]         = 236 * 15;
    vx[2]         = 236 * 15;
    vx[3]         = 236 * 15;
    vy[0]         = -22 * 15;
    vy[1]         = -23 * 15;
    vy[2]         = -23 * 15;
    vy[3]         = -22 * 15;
    vz[0]         = -104 * 15;
    vz[1]         = -104 * 15;
    vz[2]         = -108 * 15;
    vz[3]         = -108 * 15;
    n             = 0;

    while (n < 2) {
        poly3d(osscreen_x, osscreen_y, osscreen_z, 4, 68);
        beta += 90;

        if (beta > 359) {
            beta -= 360;
        }

        change_angle_of_view();
        backup_cam_x = cam_x;
        backup_cam_z = cam_z;
        cam_z        = backup_cam_x - 236 * 15;
        chry         = -18 * 15;
        chcol        = 152;
        k            = 0;

        for (j = 0; j < 7; j++) {
            cam_x = -backup_cam_z - 105 * 15;

            for (i = 0; i < 21; i++) {
                c = osscreen[n][k + i];

                if (c > 48 && c < 91) {
                    digit_at(c, 0, chry, 5.5, chcol, 0);
                } else {
                    if (c == '(') {
                        chcol = 191;
                    }

                    if (c != '$' && c != '[' && c != ']' && c != '*' && c != '&' && c != '_') {
                        digit_at(c, 0, chry, 5.5, chcol, 0);
                    } else {
                        digit_at(c, 0, chry, 6.5, 138, 0);
                    }

                    if (c == ')') {
                        chcol = 152;
                    }
                }

                cam_x -= 2.35 * 15;
            }

            k += 21;
            chry += 4.5 * 15;
        }

        cam_x = backup_cam_x;
        cam_z = backup_cam_z;
        beta  = backup_beta;
        change_angle_of_view();

        if (n == active_screen) {
            poly3d(vx, vy, vz, 4, 63);
        } else {
            poly3d(vx, vy, vz, 4, 00);
        }

        cam_z += 54 * 15;
        n++;
    }

    // Tracing the planetary map.
    // After the GOES screen.
    H_MATRIXS = 3;
    V_MATRIXS = 2;
    change_txm_repeating_mode();

    if (landing_point) {
        show_planetary_map();
        polymap(osscreen_x, osscreen_y, osscreen_z, 4, 0);
        sprintf((char *) short_text, "LQ %03d:%03d", landing_pt_lon, landing_pt_lat);
        status((char *) short_text, 10);
    } else {
        poly3d(osscreen_x, osscreen_y, osscreen_z, 4, 4);
    }

#define surface_crosshair_x_shift +25
#define surface_crosshair_y_shift -10
#define surface_crosshair_x_spacing +11
#define surface_crosshair_y_spacing +10
    setfx(2);
    stick3d(osscreen_x[0], osscreen_y[0],
            osscreen_z[0] - 27 * 15 - surface_crosshair_x_spacing + surface_crosshair_x_shift, osscreen_x[0],
            osscreen_y[1], osscreen_z[0] - 27 * 15 - surface_crosshair_x_spacing + surface_crosshair_x_shift);
    stick3d(osscreen_x[0], osscreen_y[0],
            osscreen_z[0] - 27 * 15 + surface_crosshair_x_spacing + surface_crosshair_x_shift, osscreen_x[0],
            osscreen_y[1], osscreen_z[0] - 27 * 15 + surface_crosshair_x_spacing + surface_crosshair_x_shift);
    stick3d(osscreen_x[0], osscreen_y[0] + 17 * 15 - surface_crosshair_y_spacing + surface_crosshair_y_shift,
            osscreen_z[0], osscreen_x[0],
            osscreen_y[0] + 17 * 15 - surface_crosshair_y_spacing + surface_crosshair_y_shift, osscreen_z[2]);
    stick3d(osscreen_x[0], osscreen_y[0] + 17 * 15 + surface_crosshair_y_spacing + surface_crosshair_y_shift,
            osscreen_z[0], osscreen_x[0],
            osscreen_y[0] + 17 * 15 + surface_crosshair_y_spacing + surface_crosshair_y_shift, osscreen_z[2]);
    resetfx();

    if (active_screen == 2) {
        poly3d(vx, vy, vz, 4, 63);
    } else {
        poly3d(vx, vy, vz, 4, 00);
    }

    // Finish screen tracking.
    cam_z -= 2 * 54 * 15;
    txtr = p_background;

    if (force_update) {
        force_update = 0;
    }

    // Tracing of the internal lamp.
    cam_x -= 3395;
    cam_y += 480;
    cam_z += 200;
    alogena();
    cam_x += 3395;
    cam_y -= 480;
    cam_z -= 200;

    // Tracing of panoramic domes when closing to form capsule.
    if (opencapcount != 0.0) {
        chry = cam_y;
        cam_z += 3100;
        cam_y = chry + opencapcount * 9.55f - 550;
        polycupola(-opencapcount / 85, 0);
        setfx(0);
        cupola(-opencapcount / 85, 8);
        resetfx();
        cam_y = chry - opencapcount * 9.55f;

        if (!ontheroof) {
            polycupola(+1, 0);
            setfx(0);
            cupola(+1, 8);
            resetfx();
        }

        cam_z -= 3100;
        cam_y = chry;
    }
}

/* Draw a starlight, seen from the outside */

void other_vehicle_at(double ovhx, double ovhy, double ovhz) {
    cam_x = (float) -ovhx;
    cam_y = (float) -ovhy;
    cam_z = (float) -ovhz;
    cam_z += 3100;
    setfx(2);

    if (ovhy > -375) {
        cupola(+1, 8);
    }

    if (ovhy < +375) {
        cupola(-1, 8);
    }

    resetfx();
    cam_z -= 3100;
    cam_x = 0;
    cam_y = 0;
    cam_z = 0;
    drawpv(vehicle_handle, 0, 0, (float) ovhx, (float) ovhy, (float) ovhz, 1);
    cam_x = (float) -ovhx;
    cam_y = (float) -ovhy;
    cam_z = (float) -ovhz;
    cam_z += 3100;
    setfx(2);

    if (ovhy > +375) {
        cupola(+1, 8);
    }

    if (ovhy < -375) {
        cupola(-1, 8);
    }

    resetfx();
    cam_z -= 3100;
    lens_flares_for(cam_x, cam_y, cam_z, 3225, 0, 0, -5e5, 3, 1, 1, 1, 1);
    lens_flares_for(cam_x, cam_y, cam_z, -3225, 0, 0, -5e5, 3, 1, 1, 1, 1);
    lens_flares_for(cam_x, cam_y, cam_z, 3225, 0, -6150, -5e5, 3, 1, 1, 1, 1);
    lens_flares_for(cam_x, cam_y, cam_z, -3225, 0, -6150, -5e5, 3, 1, 1, 1, 1);
}

/* Fine roba importata da noctis-0.cpp */

/* Global variables for general use. */

int8_t aso_countdown  = 100; // counter for the function "autoscreenoff"
int32_t tgt_label_pos = -1;  // selected target label position
int16_t tgts_in_show  = 0;   // targets currently displayed

/* Cartography management data. */

int8_t targets_in_range = 0;
int32_t sm_consolidated = 0;

int8_t target_name[4][24];

int8_t iptargetstring[11];
int8_t iptargetchar = 0;
int8_t iptargetplanet;
int8_t iptargetmoon;

double ap_target_id = 12345, ap_target_previd = 54321;
double current_planet_id = 12345, prev_planet_id = 54321;
int8_t labstar = 0, labplanet = 0, labstar_char = 0, labplanet_char = 0;
int32_t star_label_pos = -1, planet_label_pos = -1;

double star_id           = 12345;
int8_t star_label[25]    = "UNKNOWN STAR / CLASS ...";
int8_t star_no_label[25] = "UNKNOWN STAR / CLASS ...";

double planet_id           = 12345;
int8_t planet_label[25]    = "NAMELESS PLANET / N. ...";
int8_t planet_no_label[25] = "NAMELESS PLANET / N. ...";
int8_t moon_no_label[25]   = "NAMELESS MOON #../../...";

const char *sr_message = "SYSTEM RESET";

void update_star_label() {
    if (ap_targetted == -1) {
        strcpy((char *) star_label, "- DIRECT PARSIS TARGET -");
    } else {
        ap_target_id = ap_target_x / 100000 * ap_target_y / 100000 * ap_target_z / 100000;

        if (ap_target_id != ap_target_previd) {
            ap_target_previd = ap_target_id;
            star_label_pos   = search_id_code(ap_target_id, 'S');

            if (star_label_pos != -1) {
                FILE *smh = fopen(starmap_file, "rb");
                fseek(smh, star_label_pos, SEEK_SET);
                fread(&star_id, 8, 1, smh);
                fread(&star_label, 24, 1, smh);
                fclose(smh);
            } else {
                memcpy(star_label, star_no_label, 24);
            }

            brtl_srand(legacy_u16_from_double(ap_target_id));
            sprintf((char *) (star_label + 21), "S%02d", brtl_random(star_classes));
        }
    }
}

void update_planet_label() {
    current_planet_id = nearstar_identity + ip_targetted + 1;

    if (current_planet_id != prev_planet_id) {
        prev_planet_id   = current_planet_id;
        planet_label_pos = search_id_code(current_planet_id, 'P');

        if (planet_label_pos != -1) {
            FILE *smh = fopen(starmap_file, "rb");
            fseek(smh, planet_label_pos, SEEK_SET);
            fread(&planet_id, 8, 1, smh);
            fread(&planet_label, 24, 1, smh);
            fclose(smh);
        } else {
            if (nearstar_p_owner[ip_targetted] == -1) {
                memcpy(planet_label, planet_no_label, 24);
            } else {
                memcpy(planet_label, moon_no_label, 24);
                sprintf((char *) (planet_label + 15), "%02d", nearstar_p_moonid[ip_targetted] + 1);
                sprintf((char *) (planet_label + 18), "%02d", nearstar_p_owner[ip_targetted] + 1);
                planet_label[17] = '/';
                planet_label[20] = '&';
            }
        }

        sprintf((char *) (planet_label + 21), "P%02d", ip_targetted + 1);
    }
}

// Control the flight (Flight Control System).
void fcs() {
    int16_t n;

    if (ip_targetted != -1) {
        cline(1, "local target: ");

        if (nearstar_p_owner[ip_targetted] > -1) {
            other("moon #");
            other(alphavalue(nearstar_p_moonid[ip_targetted] + 1));
            other(" of ");
            n = nearstar_p_owner[ip_targetted];
        } else {
            n = ip_targetted;
        }

        other((char *) ord[n + 1]);
        other(" planet. ");
        other((char *) planet_description[nearstar_p_type[ip_targetted]]);
    }

    if (ap_targetted) {
        if (ap_targetted == 1) {
            cline(2, "remote target: class ");
            other(alphavalue(ap_target_class));
            other(" star; ");
            other((char *) star_description[ap_target_class]);
        } else {
            cline(2, "direct parsis target: non-star type.");
        }
    } else {
        cline(2, "no remote target selected");
    }

    cline(3, "current range: elapsed ");
    float xx = (float) pwr - 15000;

    if (xx < 0) {
        xx = 0;
    }

    other(alphavalue(xx));
    other(" kilodyams, remaining lithium: ");
    other(alphavalue(charge));
    other(" grams.");
    command(0, "set remote target");

    if (stspeed) {
        command(1, "stop vimana flight");
    } else {
        command(1, "start vimana flight");
    }

    if (landing_point) {
        command(3, "cancel landing request");
    } else {
        command(3, "deploy surface capsule");
    }

    if (ip_targetted == -1 || ip_reached) {
        command(2, "set local target");
    } else {
        if (ip_reaching) {
            command(2, "stop fine approach");
        } else {
            command(2, "start fine approach");
            command(3, "clear local target");
        }
    }
}

/* FCS Commands. */

void fcs_commands() {
    noctis::play_cockpit_button();
    switch (s_command) {
    case 1:
        if (stspeed || manual_target) {
            status("CONFLICT", 50);
            noctis::play_goesnet_chime(false);
            break;
        }

        status("TGT-REMOTE", 50);
        noctis::play_goesnet_chime(true);
        ap_targetting = 1;
        ap_targetted  = 0;
        break;

    case 2:
        if (stspeed) {
            stspeed               = 0;
            g_active_travel_speed = 0.0f;
            g_active_travel_phase = noctis::TravelPhase::arrived;
            status("IDLE", 50);
        } else {
            if (lithium_collector || manual_target) {
                status("CONFLICT", 50);
                noctis::play_goesnet_chime(false);
                break;
            }

            if (pwr > 15000) {
                stspeed               = 1;
                g_active_travel_phase = noctis::TravelPhase::charging;
                g_active_travel_speed = 0.0f;

                if (ap_targetted) {
                    nsnp         = 1;
                    ap_reached   = 0;
                    ip_reached   = 0;
                    ip_targetted = -1;
                }
            }
        }

        break;

    case 3:
        if (ip_reached || ip_targetted == -1) {
            if (ap_reached) {
                status("TGT-LOCAL", 50);
                noctis::play_goesnet_chime(true);
                ip_targetted  = -1;
                ip_targetting = 1;
                ip_reaching   = 0;
                ip_reached    = 0;
                iptargetchar  = 0;
            } else {
                status("NEED RECAL", 75);
                noctis::play_goesnet_chime(false);
            }
        } else {
            if (ip_reaching) {
                status("IDLE", 50);
                ip_targetted          = -1;
                ip_reaching           = 0;
                ip_reached            = 1;
                g_active_travel_speed = 0.0f;
                g_active_travel_phase = noctis::TravelPhase::arrived;
            } else {
                if (pwr > 15000) {
                    ip_reaching           = 1;
                    g_active_travel_phase = noctis::TravelPhase::warming_up;
                    g_active_travel_speed = 0.0f;
                    status("CONFIRM", 50);
                    noctis::play_goesnet_chime(true);
                }
            }
        }

        break;

    case 4:
        if (!ip_reaching && ip_targetted != -1) {
            if (!ip_reached) {
                ip_targetted = -1;
                status("TGT REJECT", 50);
                noctis::play_goesnet_chime(false);
            } else {
                landing_point = 1 - landing_point;

                if (landing_point) {
                    if (nearstar_p_type[ip_targetted] == 0 || nearstar_p_type[ip_targetted] == 6 ||
                        nearstar_p_type[ip_targetted] >= 9) {
                        status("IMPOSSIBLE", 50);
                        noctis::play_goesnet_chime(false);
                        landing_point = 0;
                    } else {
                        status("SURFACE", 50);
                        noctis::play_goesnet_chime(true);
                        landing_pt_lon = 0;
                        landing_pt_lat = 60;
                    }
                } else {
                    status("IDLE", 50);
                }
            }
        } else {
            status("ERROR", 50);
        }
        break;
    default:
        break;
    }
}

/* Onboard devices: main menu and four submenus */

void devices() {
    double parsis_x, parsis_y, parsis_z;
    int16_t n, sp;

    switch (dev_page) {
    case 0: // sub menu
        command(0, "navigation instruments");
        command(1, "miscellaneous");
        command(2, "galactic cartography");
        command(3, "emergency functions");
        cline(3, "SELECT ARGUMENT");
        break;

    case 1: // navigation status
        if (field_amplificator) {
            command(0, "STARFIELD AMPLIFICATOR");
            cline(1, "starfield amplification active, ");
        } else {
            command(0, "starfield amplificator");
            cline(1, "starfield amplification disabled, ");
        }

        if (anti_rad) {
            command(3, "FORCE RADIATIONS LIMIT");
            other("high-radiation fields are avoided.");
        } else {
            command(3, "force radiations limit");
            other("high-radiation fields are ignored.");
        }

        if (nsync) {
            if (ip_targetted != -1 && ip_reached) {
                sp = 0;

                for (n = 0; n < ip_targetted; n++) {
                    if (nearstar_p_type[n] != -1) {
                        sp++;
                    }
                }

                cline(1, "tracking status: performing ");

                if (nsync == 1) {
                    other("fixed-point chase.");
                    command(2, "fixed-point chase");
                }

                if (nsync == 2) {
                    other("far chase.");
                    command(2, "far chase");
                }

                if (nsync == 3) {
                    other("syncrone orbit.");
                    command(2, "syncrone orbit");
                }

                if (nsync == 4) {
                    other("high-speed orbit.");
                    command(2, "high-speed orbit");
                }

                if (nsync == 5) {
                    other("near chase.");
                    command(2, "near chase");
                }
            } else {
                cline(2, "tracking status: disconnected.");

                if (nsync == 1) {
                    command(2, "fixed-point chase");
                }

                if (nsync == 2) {
                    command(2, "far chase");
                }

                if (nsync == 3) {
                    command(2, "syncrone orbit");
                }

                if (nsync == 4) {
                    command(2, "high-speed orbit");
                }

                if (nsync == 5) {
                    command(2, "near chase");
                }
            }
        } else {
            cline(2, "tracking status: inactive.");
            command(2, "drive tracking mode");
        }

        if (pl_search) {
            command(1, "LOCAL PLANETS FINDER");
            float xx = (float) (nearstar_x - dzat_x);
            float yy = (float) (nearstar_y - dzat_y);
            float zz = (float) (nearstar_z - dzat_z);
            xx       = sqrt(xx * xx + yy * yy + zz * zz);

            if (xx < 20000) {
                if (nearstar_nop) {
                    cline(3, "planet finder report: system has ");
                    other(alphavalue(nearstar_nop));
                    other(" ");

                    if (nearstar_class == 9) {
                        other("proto");
                    }

                    if (nearstar_nop == 1) {
                        other("planet, and ");
                    } else {
                        other("planets, and ");
                    }

                    other(alphavalue(nearstar_nob - nearstar_nop));
                    other(" minor bodies. ");
                    other(alphavalue(nearstar_labeled));
                    other(" labeled out of ");
                    other(alphavalue(nearstar_nob));
                    other(".");
                } else {
                    cline(3, "planet finder report: there are no major bodies "
                             "in this system.");
                }
            } else {
                cline(3, "planet finder report: no stellar systems within "
                         "remote sensors range.");
            }
        } else {
            command(1, "local planets finder");
        }

        break;

    case 2: // miscellaneous devices status
        if (ilightv == 1) {
            command(0, "internal light on");
        } else {
            command(0, "internal light off");
        }

        command(1, "remote target data");
        command(2, "local target data");
        command(3, "environment data");

        if (data == 1) {
            command(1, "REMOTE TARGET DATA");
        }

        if (data == 2) {
            command(2, "LOCAL TARGET DATA");
        }

        if (data == 3) {
            command(3, "ENVIRONMENT DATA");
        }

        break;

    case 3: { // galactic cartography status
        if (labstar) {
            command(0, "assign star label");
        } else {
            if (star_label_pos > -1) {
                command(0, "remove star label");
            } else {
                command(0, "label star as...");
            }
        }

        if (labplanet) {
            command(1, "assign planet label");
        } else {
            if (planet_label_pos > -1) {
                command(1, "remove planet label");
            } else {
                command(1, "label planet as...");
            }
        }

        if (targets_in_range) {
            command(2, "quit targets in range");
        } else {
            command(2, "show targets in range");
        }

        if (manual_target) {
            command(3, "(enter coordinates)");
        } else {
            command(3, "set target to parsis");
        }

        cline(1, "epoc ");
        other(alphavalue((int32_t) epoc));
        other(" triads ");
        const auto triads = noctis::split_triad_time(secs);
        other(noctis::format_triad(triads.sinister).c_str());
        other(",");
        other(noctis::format_triad(triads.medius).c_str());
        other(",");
        other(noctis::format_triad(triads.dexter).c_str());
        cline(2, "parsis universal coordinates: ");
        parsis_x = round(dzat_x);
        parsis_y = round(dzat_y);
        parsis_z = round(dzat_z);
        other(alphavalue(parsis_x));
        other(";");
        other(alphavalue(-parsis_y));
        other(";");
        other(alphavalue(parsis_z));
        cline(3, "heading pitch: ");
        other(alphavalue((int16_t) (sin(deg * navigation_beta) * +100)));
        other(";");
        other(alphavalue((int16_t) (cos(deg * navigation_beta) * -100)));
        break;
    }

    case 4: // emergency functions status
        command(0, "reset onboard system");
        command(1, "send help request");

        if (lithium_collector) {
            command(2, "stop scoping lithium");
        } else {
            command(2, "scope for lithium");
        }

        command(3, "clear status");

        if (gburst == -1) {
            cline(1, "NOTE: there are no emergencies at the moment.");
            cline(2, "help request not sent.");
        }
        break;
    default:
        break;
    }
}

/* On-board device controls. */

int8_t dummy_identity[9] = "Removed:";
int8_t comp_data[32];

void dev_commands() {
    noctis::play_cockpit_button();
    int16_t n;
    float dist;

    switch (dev_page) {
    case 0:
        dev_page = s_command;

        switch (s_command) {
        case 1:
            status("NAVIGATION", 50);
            break;
        case 2:
            status("SUPPORTS", 50);
            break;
        case 3:
            status("CARTOGRAFY", 50);
            break;
        case 4:
            status("EMERGENCY", 50);
            break;
        default:
            break;
        }

        break;

    case 1:
        switch (s_command) {
        case 1:
            field_amplificator = 1 - field_amplificator;

            if (field_amplificator) {
                status("ACTIVE", 50);
            } else {
                status("INACTIVE", 50);
            }

            break;

        case 2:
            pl_search = 1 - pl_search;

            if (pl_search) {
                status("ACTIVE", 50);
            } else {
                status("INACTIVE", 50);
            }

            break;

        case 3:
            nsync++;
            nsync %= 6;

            if (!nsync) {
                status("IDLE", 50);
                ip_reaching = 0;
                ip_reached  = 1;
            } else {
                status("ACQUIRED", 50);

                if (ip_reached) {
                    ip_reaching = 1;
                    ip_reached  = 0;
                }
            }

            break;

        case 4:
            anti_rad = 1 - anti_rad;

            if (anti_rad) {
                status("ACTIVE", 50);
            } else {
                status("INACTIVE", 50);
            }
            break;
        default:
            break;
        }
        break;

    case 2:
        switch (s_command) {
        case 1:
            ilightv = -ilightv;

            if (ilightv > 0) {
                status("ON", 50);
            } else {
                status("OFF", 50);
            }

            break;

        case 2:
            if (data == 1) {
                datasheetdelta = -2;
            } else {
                data           = 1;
                datasheetdelta = +2;
            }

            break;

        case 3:
            if (data == 2) {
                datasheetdelta = -2;
            } else {
                data           = 2;
                datasheetdelta = +2;
            }

            break;

        case 4:
            if (data == 3) {
                datasheetdelta = -2;
            } else {
                data           = 3;
                datasheetdelta = +2;
            }

            break;
        default:
            break;
        }

        break;

    case 3: // galactic cartography commands
        switch (s_command) {
        case 1:
            if (ap_targetted == 1 && !ap_targetting && !labplanet) {
                labstar = 1 - labstar;

                if (labstar) {
                    if (star_label_pos > -1) {
                        labstar = 0;

                        if (star_label_pos >= sm_consolidated) {
                            const auto removed = noctis::remove_starmap_label(starmap_file, star_label_pos);
                            if (removed.status == noctis::GoesDataStatus::ok) {
                                ap_target_previd = 12345;
                                status("REMOVED", 50);
                                star_label_pos = -1;
                                nearstar_labeled--;
                            } else {
                                status("INT. ERROR", 50);
                            }
                        } else {
                            status("DENIED", 50);
                        }
                    } else {
                        status("PROMPT", 50);
                        labstar_char = 0;
                        star_id      = ap_target_id;

                        for (n = 0; n < 21; n++) {
                            star_label[n] = 32;
                        }
                    }
                } else {
                    std::string name(reinterpret_cast<char *>(star_label), 20);
                    while (!name.empty() && name.back() == ' ')
                        name.pop_back();
                    const auto assigned = noctis::append_starmap_label(
                        starmap_file, star_id, name, noctis::GoesObjectKind::star,
                        static_cast<std::int16_t>((star_label[22] - '0') * 10 + star_label[23] - '0'), star_label_pos);
                    if (assigned.status == noctis::GoesDataStatus::ok) {
                        status("ASSIGNED", 50);
                        nearstar_labeled++;
                        ap_target_previd = 12345;
                        noctis::active_flight_log().record_label_assigned(
                            star_id, name, false,
                            static_cast<std::int16_t>((star_label[22] - '0') * 10 + star_label[23] - '0'));
                        noctis::active_flight_log().save_to_file(noctis::runtime_paths().data_dir / "flight_log.json");
                    } else if (assigned.status == noctis::GoesDataStatus::rejected) {
                        status("EXTANT", 50);
                        ap_target_previd = 12345;
                        star_label_pos   = -1;
                    } else {
                        status("INT. ERROR", 50);
                    }
                }
            } else {
                status("CONFLICT", 50);
            }

            break;

        case 2:
            if (ip_targetted != -1 && !labstar) {
                labplanet = 1 - labplanet;

                if (labplanet) {
                    if (planet_label_pos > -1) {
                        labplanet = 0;

                        if (planet_label_pos >= sm_consolidated) {
                            const auto removed = noctis::remove_starmap_label(starmap_file, planet_label_pos);
                            if (removed.status == noctis::GoesDataStatus::ok) {
                                prev_planet_id = 12345;
                                status("REMOVED", 50);
                                planet_label_pos = -1;
                                nearstar_labeled--;
                            } else {
                                status("INT. ERROR", 50);
                            }
                        } else {
                            status("DENIED", 50);
                        }
                    } else {
                        status("PROMPT", 50);
                        labplanet_char = 0;
                        planet_id      = nearstar_identity + ip_targetted + 1;

                        for (n = 0; n < 21; n++) {
                            planet_label[n] = 32;
                        }
                    }
                } else {
                    std::string name(reinterpret_cast<char *>(planet_label), 20);
                    while (!name.empty() && name.back() == ' ')
                        name.pop_back();
                    const auto assigned =
                        noctis::append_starmap_label(starmap_file, planet_id, name, noctis::GoesObjectKind::planet,
                                                     static_cast<std::int16_t>(ip_targetted + 1), planet_label_pos);
                    if (assigned.status == noctis::GoesDataStatus::ok) {
                        status("ASSIGNED", 50);
                        nearstar_labeled++;
                        prev_planet_id = 12345;
                        noctis::active_flight_log().record_label_assigned(
                            planet_id, name, true, static_cast<std::int16_t>(ip_targetted + 1));
                        noctis::active_flight_log().save_to_file(noctis::runtime_paths().data_dir / "flight_log.json");
                    } else if (assigned.status == noctis::GoesDataStatus::rejected) {
                        status("EXTANT", 50);
                        prev_planet_id   = 12345;
                        planet_label_pos = -1;
                    } else {
                        status("INT. ERROR", 50);
                    }
                }
            } else {
                status("CONFLICT", 50);
            }

            break;

        case 3:
            targets_in_range = 1 - targets_in_range;

            if (targets_in_range) {
                targets             = 0;
                target_line         = 0;
                topmost_target      = 0;
                tgt_collect_lastpos = 4;
                collecting_targets  = 1;
                memset(&target_name[0], 32, 4 * 24);
                collect_targets();
                tgts_in_show = 0;
            }

            break;

        case 4: // target parsis
            manual_target = 1 - manual_target;

            if (manual_target) {
                mt_coord           = 0;
                mt_string_char     = 0;
                manual_x_string[0] = 0;
                status("TGT MANUAL", 100);
                ap_targetted = 0;
                stspeed      = 0;
            } else {
                status("TGT REJECT", 100);
            }

            break;

        default:
            break;
        }

        break;

    case 4:
        switch (s_command) {
        case 1: // reset onboard system
            reset_signal = 150;
            break;

        case 2: // help request
            if (pwr <= 15000 && !charge) {
                dist = fabs(dzat_x + dzat_y + dzat_z) * 0.0001;

                if (dist > 1800) {
                    dist = 1800;
                }

                fast_srand(static_cast<uint32_t>(simulation_ticks()));
                helptime = (flandom() * dist) + secs + 60;
                status("HELP REQ.", 50);
                gburst = 63;
            } else {
                status("ERROR", 50);
                gburst = -1;
            }

            break;

        case 3: // collect lithium
            if (!lithium_collector) {
                if (ap_reached && ap_targetted == 1) {
                    if (nearstar_class == 5 || (nearstar_class == 6 && nearstar_ray > 4)) {
                        lithium_collector = 1;
                    } else {
                        status("UNSUITABLE", 50);
                    }
                } else {
                    status("NEED RECAL", 75);
                }
            } else {
                lithium_collector = 0;
                status("IDLE", 50);
            }

            break;

        case 4: // clear status
            status("READY", 50);
            gburst = 0;
            break;
        default:
            break;
        }
    default:
        break;
    }
}

/* Preferences. */

void prefs() {
    command(0, noctis::ship_preference_label(noctis::ShipPreference::automatic_screen_sleep, autoscreenoff));
    command(1, noctis::ship_preference_label(noctis::ShipPreference::reversed_pitch, revcontrols));
    command(2, noctis::ship_preference_label(noctis::ShipPreference::persistent_menus, menusalwayson));
    command(3, noctis::ship_preference_label(noctis::ShipPreference::polarized_hull, depolarize));
}

/* Controls for setting preferential options. */

void toggle_option(int8_t *option_flag) {
    *option_flag = 1 - *option_flag;

    if (*option_flag) {
        status("ACQUIRED", 50);
    } else {
        status("DISABLED", 50);
    }
}

void pfs_commands() {
    switch (s_command) {
    case 1:
        toggle_option(&autoscreenoff);
        break;

    case 2:
        toggle_option(&revcontrols);
        break;

    case 3:
        toggle_option(&menusalwayson);
        break;

    case 4:
        toggle_option(&depolarize);
        break;
    default:
        break;
    }
}

/*  Commands given to the on-board computer.
 *  All except "disable screen". */

void commands() {
    switch (sys) {
    case 1:
        fcs_commands();
        break;

    case 2:
        dev_commands();
        break;

    case 3:
        pfs_commands();
        break;

    default:
        break;
    }
}

/*  Undo the situation, reproducing it in all respects,
     and making it evolve at the current time.
 *  Note: Garbage translation above
 *
 *  This loads the situation saved by freeze() */

bool unfreeze() {
    double elapsed, dpwr;
    // Reading the consolidated starmap.
    FILE *smh = fopen(starmap_file, "rb+");

    if (smh != nullptr) {
        fread(&sm_consolidated, 4, 1, smh);

        if (!sm_consolidated) {
            sm_consolidated = fseek(smh, 0, SEEK_END);
            fseek(smh, 0, SEEK_SET);
            fwrite(&sm_consolidated, 4, 1, smh);
        }

        fclose(smh);
    } else {
        sm_consolidated = 0;
    }

    noctis::active_flight_log().load_from_file(noctis::runtime_paths().data_dir / "flight_log.json");

    noctis::NativeSaveState native_state;
    bool is_legacy_migration = false;
    const auto native_result = noctis::load_native_save(native_situation_file, native_state);
    if (native_result.status == noctis::NativeSaveStatus::ok) {
        apply_native_state(native_state);
    } else {
        if (native_result.status != noctis::NativeSaveStatus::not_found) {
            noctis::log_event("error", "native_save", native_result.message);
            return false;
        }

        noctis::LegacySituationImport imported;
        const auto legacy_result = noctis::load_legacy_situation(situation_file, imported);
        if (legacy_result.status == noctis::NativeSaveStatus::not_found) {
            synchronize_secs_to_wall_clock();
            npcs = -12345;
            prepare_nearstar();
            return true;
        }
        if (legacy_result.status != noctis::NativeSaveStatus::ok) {
            noctis::log_event("error", "legacy_save", legacy_result.message);
            return false;
        }
        const auto migration_result = noctis::save_native_save(native_situation_file, imported.state);
        if (migration_result.status != noctis::NativeSaveStatus::ok) {
            noctis::log_event("error", "legacy_migration", migration_result.message);
            return false;
        }
        apply_native_state(imported.state);
        is_legacy_migration = true;
        noctis::log_event("info", "legacy_migration",
                          std::string("migrated ") + std::string(noctis::legacy_layout_name(imported.layout)) +
                              " situation to native v1");
    }

    /* Resynchronization of the situation
     * 	in relation to previous events
     * 	(hidden evolution of the situation). */
    const double saved_secs = secs;
    synchronize_secs_to_wall_clock();
    if (is_legacy_migration || saved_secs < 1e8) {
        elapsed = 0.0;
        if (helptime != 0.0) {
            helptime = 0.0;
        }
    } else {
        elapsed = secs - saved_secs;
        if (elapsed < 0.0) {
            elapsed = 0.0;
        }
    }

    if ((helptime != 0.0) && (secs > (helptime + 20))) {
        helptime = 0;
        charge   = 4;
        gburst   = 0;
    }

    /* Reconstruction of the current star system. */
    npcs   = -12345;
    _delay = 0;
    prepare_nearstar();

    if (noctis::active_flight_log().entries().empty()) {
        std::string init_sname(reinterpret_cast<const char *>(star_label), 20);
        while (!init_sname.empty() && init_sname.back() == ' ') init_sname.pop_back();
        noctis::active_flight_log().record_system_arrival(
            nearstar_x, nearstar_y, nearstar_z, nearstar_identity,
            init_sname, nearstar_class, 0.0);
        noctis::active_flight_log().save_to_file(noctis::runtime_paths().data_dir / "flight_log.json");
    }

    if (lithium_collector) {
        while (elapsed >= 30 && charge < 120) {
            elapsed -= 30;
            charge++;
        }

        if (charge == 120) {
            pwr = 20000;
        } else {
            brtl_srand(legacy_u16_from_double(secs));
            pwr = (int16_t) (brtl_random(5000) + 15000);
        }
    }

    /* Update on additional consumption. */
    dpwr = pwr;

    if (ilightv == 1) {
        dpwr -= elapsed / 84;
    }

    if (pl_search) {
        dpwr -= elapsed / 155;
    }

    if (field_amplificator) {
        dpwr -= elapsed / 41;
    }

    if (ip_targetted != -1 && ip_reached) {
        if (nsync == 1) { // fixed-point chase
            dpwr -= elapsed / 29;
        }

        if (nsync == 2) { // far chase
            dpwr -= elapsed / 18;
        }

        if (nsync == 3) { // syncrone orbit
            dpwr -= elapsed / 58;
        }

        if (nsync == 4) { // vimana orbit
            dpwr -= elapsed / 7;
        }

        if (nsync == 5) { // near chase
            dpwr -= elapsed / 33;
        }
    }

    while (dpwr < 15000) {
        if (charge > 0) {
            dpwr += 5000;
            charge--;
        } else if (charge < 0) {
            dpwr = 20000;
        } else {
            dpwr = 15000;
        }
    }

    pwr = (int16_t) dpwr;
    return true;
}

/* Main program. */

float starmass_correction[star_classes] = {
    1.886,   // Class 0
    1.50,    // Class 1
    8000.40, // Class 2
    0.05,    // Class 3
    2.44,    // Class 4
    3.10,    // Class 5
    9.30,    // Class 6
    48.00,   // Class 7
    1.00,    // Class 8
    1.00,    // Class 9
    0.07,    // Class 10
    15000.00 // Class 11
};

Texture2D screen_texture;
Texture2D screen_texture_2x;

// Actual noctis stuff starts here.
float satur, DfCoS;

// float user_drawing_range;
int32_t ir, ig, ib, ire = 0, ige = 0, ibe = 0;
int32_t ir2, ig2, ib2, ir2e = 0, ig2e = 0, ib2e = 0;
int32_t ir3, ig3, ib3, ir3e = 0, ig3e = 0, ib3e = 0;
int16_t mc            = 0;
uint8_t p_mpul        = 0;
int8_t sky_palette_ok = 0;
int8_t lselect, rselect, lrv;
bool right_dblclick = false;
float right_dblclick_dir;
double dpz, ras, rap, dasp, eclipse;
double dxx, dyy, dzz, l_dsd, p_dsd, stz, ang;
static int16_t opencapcount = 0;
int16_t opencapdelta        = 0;
int16_t holdtomiddle        = 0;
int8_t leftturn, rightturn, arrowcolor, farstar = 0;
char temp_distance_buffer[16];
uint32_t pqw;
float hold_z;
float tmp_float;
int32_t p1, p2, p3, p4;

std::chrono::steady_clock::time_point right_dblclick_timing{};

int16_t resolve                  = 64;
std::uint32_t last_snapshot      = UINT32_MAX;
std::int8_t option_mouse_look    = 0;
std::int16_t roof_speed          = 0;
std::int8_t draw_hud             = 1;
std::int8_t suit_torch           = 0;
std::int8_t lens_flare_mode      = 0;
std::int8_t seamless_border      = 0;
std::int8_t graphics_menu_status = 0;
std::int8_t about                = 0;
noctis::MovieRecorder movie_recorder;
bool surface_fixture_mode            = false;
bool landing_fixture_mode            = false;
bool environment_fixture_mode        = false;
bool content_fixture_mode            = false;
bool orbit_surface_fixture_mode      = false;
bool oakenshield_fixture_mode        = false;
const char *surface_fixture_name     = "felysia-habitable";
const char *environment_fixture_name = "felysia-habitable";

void handle_movie_extended_key(std::int16_t key) {
    if (key == 0x3D) {
        movie_recorder.toggle_menu();
        graphics_menu_status = 0;
        about                = 0;
        status(movie_recorder.menu_open() ? "MOVIEMAKER" : "MVMENU OFF", 100);
        return;
    }
    if (!movie_recorder.menu_open() || movie_recorder.session_active())
        return;
    if (key == 144 || key == 142) {
        movie_recorder.change_deck(key == 144 ? 1 : -1);
        status(movie_recorder.deck_occupied(noctis::runtime_paths().movies_dir) ? "DECK EXISTS" : "DECK FREE", 100);
    }
}

bool handle_movie_key(std::int16_t key, bool label_entry) {
    if (label_entry)
        return false;
    if (key == 'p' && movie_recorder.session_active()) {
        movie_recorder.pause_or_resume();
        status(movie_recorder.paused() ? "PAUSE REC" : "RESUME REC", 100);
        return true;
    }
    if (key == 13 && (movie_recorder.menu_open() || movie_recorder.session_active())) {
        if (movie_recorder.recording()) {
            movie_recorder.stop();
            status("STOP REC", 100);
            return true;
        }
        const auto result = movie_recorder.start_or_resume(noctis::runtime_paths().movies_dir);
        if (result == noctis::MovieStartResult::started)
            status("RECORDING", 100);
        else if (result == noctis::MovieStartResult::resumed)
            status("RESUME REC", 100);
        else if (result == noctis::MovieStartResult::occupied)
            status("DECK EXISTS", 100);
        else
            status("MOVIE ERROR", 100);
        return true;
    }
    if (!movie_recorder.menu_open() || movie_recorder.session_active())
        return false;
    if (key == '+' || key == '-') {
        movie_recorder.change_cadence(key == '+' ? 1 : -1);
        status("MOVIE RATE", 100);
        return true;
    }
    if (key == 'f') {
        movie_recorder.toggle_black_flash();
        status(movie_recorder.black_flash() ? "BLK FLASH" : "NO FLASH", 100);
        return true;
    }
    return false;
}

void advance_movie_capture(bool ascending_from_surface) {
    const auto decision = movie_recorder.advance_simulation_frame(ascending_from_surface);
    if (decision.capture_path) {
        const bool written = write_indexed_bmp(*decision.capture_path);
        movie_recorder.confirm_capture(written);
        if (!written) {
            status("MOVIE ERROR", 100);
        } else if (movie_recorder.black_flash()) {
            std::memset(adapted, 0, adapted_width * adapted_height);
        } else {
            std::fill(adapted + 198 * adapted_width, adapted + adapted_width * adapted_height, 127);
        }
    }
    if (decision.stopped)
        status("ASCENT CUT", 100);
}

namespace {
struct SurfaceFixtureCase {
    std::string_view name;
    double star_x;
    double star_y;
    double star_z;
    int16_t star_class;
    float star_radius;
    int16_t body_index;
    int8_t body_type;
    int16_t longitude;
    int16_t latitude;
};

constexpr SurfaceFixtureCase surface_fixture_cases[] = {
    {"felysia-rocky", -18928.0, -29680.0, -67336.0, 0, 5.021F, 4, 1, 1, 60},
    {"felysia-thick-atmosphere", -18928.0, -29680.0, -67336.0, 0, 5.021F, 7, 2, 1, 60},
    {"felysia-habitable", -18928.0, -29680.0, -67336.0, 0, 5.021F, 3, 3, 1, 60},
    {"felysia-corrugated", -18928.0, -29680.0, -67336.0, 0, 5.021F, 0, 4, 1, 60},
    {"felysia-thin-atmosphere", -18928.0, -29680.0, -67336.0, 0, 5.021F, 6, 5, 1, 60},
    {"class11-icy", -18927.0, -29680.0, -67336.0, 11, 5.021F, 8, 7, 1, 60},
    {"felysia-milky", -18928.0, -29680.0, -67336.0, 0, 5.021F, 1, 8, 1, 60},
};

const SurfaceFixtureCase *find_surface_fixture(std::string_view name) {
    for (const auto &fixture : surface_fixture_cases) {
        if (fixture.name == name) {
            return &fixture;
        }
    }
    return nullptr;
}

void select_surface_fixture(const SurfaceFixtureCase &fixture) {
    const bool seeded_felysia = fixture.star_x == -18928.0 && fixture.star_y == -29680.0 &&
                                fixture.star_z == -67336.0 && fixture.star_class == 0 && fixture.star_radius == 5.021F;
    if (!seeded_felysia) {
        ap_target_x     = fixture.star_x;
        ap_target_y     = fixture.star_y;
        ap_target_z     = fixture.star_z;
        ap_target_class = fixture.star_class;
        ap_target_ray   = fixture.star_radius;
        ap_target_spin  = fixture.star_class == 11 ? 1 : 0;
        ap_target_r     = fixture.star_class == 11 ? 0 : 63;
        ap_target_g     = fixture.star_class == 11 ? 63 : 58;
        ap_target_b     = fixture.star_class == 11 ? 63 : 40;
        _delay          = 0;
        prepare_nearstar();
    }
    ip_targetted   = fixture.body_index;
    landing_pt_lon = fixture.longitude;
    landing_pt_lat = fixture.latitude;
}

enum class LandingFixturePhase {
    descending,
    walking_out,
    walking_back,
};

LandingFixturePhase landing_fixture_phase     = LandingFixturePhase::descending;
std::uint32_t landing_fixture_frames          = 0;
std::uint32_t landing_fixture_touchdown_frame = 0;
std::uint32_t landing_fixture_return_frame    = 0;
std::uint64_t journey_orbit_hash              = 0;
std::uint64_t journey_touchdown_hash          = 0;
std::uint64_t journey_outbound_hash           = 0;
std::uint64_t journey_return_hash             = 0;
std::uint32_t journey_rendered_frames         = 0;
std::uint64_t environment_fixture_hash        = 0;
std::uint32_t environment_fixture_frames      = 0;
int16_t environment_fixture_longitude         = 1;
int16_t environment_fixture_latitude          = 60;
bool graphical_smoke_mode                     = false;
std::uint32_t graphical_smoke_frames          = 0;

std::uint64_t indexed_frame_hash() {
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (std::size_t index = 0; index < adapted_width * adapted_height; ++index) {
        hash ^= adapted[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

noctis::InputFrame scripted_landing_input() {
    noctis::InputFrame frame;
    ++landing_fixture_frames;
    if (landing_fixture_frames > 10000) {
        frame.escape_down = true;
        return frame;
    }
    if (!landed) {
        return frame;
    }

    const float capsule_x = static_cast<float>((atl_x << 14) + atl_x2);
    const float capsule_z = static_cast<float>((atl_z << 14) + atl_z2);
    const float distance  = std::hypot(pos_x - capsule_x, pos_z - capsule_z);
    if (landing_fixture_phase == LandingFixturePhase::descending) {
        landing_fixture_phase           = LandingFixturePhase::walking_out;
        landing_fixture_touchdown_frame = landing_fixture_frames;
    }
    if (landing_fixture_phase == LandingFixturePhase::walking_out) {
        if (distance < 5000.0F) {
            frame.move_forward = true;
        } else {
            landing_fixture_phase = LandingFixturePhase::walking_back;
        }
    }
    if (landing_fixture_phase == LandingFixturePhase::walking_back) {
        if (distance > 800.0F) {
            frame.move_backward = true;
        } else if (landing_fixture_return_frame == 0) {
            landing_fixture_return_frame = landing_fixture_frames;
        }
    }
    return frame;
}

noctis::InputFrame scripted_surface_frame_input() {
    noctis::InputFrame frame;
    frame.escape_down = environment_fixture_mode || environment_fixture_frames >= 2;
    return frame;
}
} // namespace

void loop();

int main(int argc, char **argv) {
    bool diagnostics_only                 = false;
    bool prepare_user_data_only           = false;
    bool native_save_fixture_mode         = false;
    bool ship_interface_fixture_mode      = false;
    bool goesnet_fixture_mode             = false;
    bool persistence_fixture_mode         = false;
    bool movie_fixture_mode               = false;
    bool no_audio_mode                    = false;
    bool reset_data_only                  = false;
    std::optional<std::filesystem::path> export_starmap_path;
    std::optional<std::filesystem::path> import_starmap_path;
    std::optional<std::filesystem::path> validate_starmap_path;
    int drive_override                    = 0;
    const char *persistence_fixture_phase = nullptr;
    std::optional<double> fixture_universe_seconds;
    std::optional<std::filesystem::path> user_data_override;
    std::optional<std::filesystem::path> migration_source;
    std::optional<bool> portable_mode_override;
    std::optional<noctis::InternalResolutionMode> resolution_override;
    for (int arg = 1; arg < argc; ++arg) {
        if (std::string_view(argv[arg]) == "--diagnostics") {
            diagnostics_only = true;
        } else if (std::string_view(argv[arg]) == "--prepare-user-data") {
            prepare_user_data_only = true;
        } else if (std::string_view(argv[arg]) == "--reset-data") {
            reset_data_only = true;
        } else if (std::string_view(argv[arg]) == "--export-starmap") {
            if (arg + 1 < argc && argv[arg + 1][0] != '-') {
                export_starmap_path = std::filesystem::path(argv[++arg]);
            } else {
                export_starmap_path = std::filesystem::path("outbox.nsm");
            }
        } else if (std::string_view(argv[arg]) == "--import-starmap" && arg + 1 < argc) {
            import_starmap_path = std::filesystem::path(argv[++arg]);
        } else if (std::string_view(argv[arg]) == "--validate-starmap" && arg + 1 < argc) {
            validate_starmap_path = std::filesystem::path(argv[++arg]);
        } else if (std::string_view(argv[arg]) == "--graphical-smoke") {
            graphical_smoke_mode = true;
        } else if (std::string_view(argv[arg]) == "--no-audio") {
            no_audio_mode = true;
        } else if (std::string_view(argv[arg]) == "--portable") {
            portable_mode_override = true;
        } else if (std::string_view(argv[arg]) == "--system-user-data") {
            portable_mode_override = false;
        } else if (std::string_view(argv[arg]) == "--omega-drive") {
            drive_override = -1;
        } else if (std::string_view(argv[arg]) == "--standard-drive") {
            drive_override = 1;
        } else if ((std::string_view(argv[arg]) == "--resolution" || std::string_view(argv[arg]) == "--internal-res") && arg + 1 < argc) {
            std::string_view res_arg = argv[++arg];
            if (res_arg == "1x" || res_arg == "1" || res_arg == "320x200") {
                resolution_override = noctis::InternalResolutionMode::res_1x;
            } else if (res_arg == "2x" || res_arg == "2" || res_arg == "640x400") {
                resolution_override = noctis::InternalResolutionMode::res_2x;
            } else if (res_arg == "4x" || res_arg == "4" || res_arg == "1280x800") {
                resolution_override = noctis::InternalResolutionMode::res_4x;
            } else {
                noctis::log_event("error", "arguments", "Invalid resolution: " + std::string(res_arg) + " (expected 1x, 2x, or 4x)");
                return 2;
            }
        } else if (std::string_view(argv[arg]) == "--user-data-dir" && arg + 1 < argc) {
            user_data_override = std::filesystem::path(argv[++arg]);
        } else if (std::string_view(argv[arg]) == "--migrate-from" && arg + 1 < argc) {
            migration_source = std::filesystem::path(argv[++arg]);
        } else if (std::string_view(argv[arg]) == "--surface-fixture") {
            surface_fixture_mode = true;
            if (arg + 1 < argc && argv[arg + 1][0] != '-') {
                surface_fixture_name = argv[++arg];
            }
        } else if (std::string_view(argv[arg]) == "--landing-fixture") {
            landing_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--oakenshield-fixture") {
            oakenshield_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--orbit-surface-fixture") {
            orbit_surface_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--native-save-fixture") {
            native_save_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--ship-interface-fixture") {
            ship_interface_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--goesnet-fixture") {
            goesnet_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--persistence-fixture" && arg + 1 < argc) {
            persistence_fixture_mode  = true;
            persistence_fixture_phase = argv[++arg];
        } else if (std::string_view(argv[arg]) == "--fixture-universe-seconds" && arg + 1 < argc) {
            fixture_universe_seconds = std::strtod(argv[++arg], nullptr);
        } else if (std::string_view(argv[arg]) == "--movie-fixture") {
            movie_fixture_mode = true;
        } else if (std::string_view(argv[arg]) == "--environment-fixture") {
            environment_fixture_mode = true;
            if (arg + 1 < argc && argv[arg + 1][0] != '-') {
                environment_fixture_name = argv[++arg];
            }
            if (arg + 2 < argc && argv[arg + 1][0] != '-' && argv[arg + 2][0] != '-') {
                environment_fixture_longitude = static_cast<int16_t>(std::strtol(argv[++arg], nullptr, 10));
                environment_fixture_latitude  = static_cast<int16_t>(std::strtol(argv[++arg], nullptr, 10));
            }
        } else if (std::string_view(argv[arg]) == "--content-fixture") {
            content_fixture_mode = true;
            if (arg + 1 < argc && argv[arg + 1][0] != '-') {
                environment_fixture_name = argv[++arg];
            }
            if (arg + 2 < argc && argv[arg + 1][0] != '-' && argv[arg + 2][0] != '-') {
                environment_fixture_longitude = static_cast<int16_t>(std::strtol(argv[++arg], nullptr, 10));
                environment_fixture_latitude  = static_cast<int16_t>(std::strtol(argv[++arg], nullptr, 10));
            }
        } else {
            noctis::log_event(
                "error", "arguments",
                "Usage: nivlr [--diagnostics|--graphical-smoke|--prepare-user-data|--reset-data] "
                "[--export-starmap [PATH]] [--import-starmap PATH] [--validate-starmap PATH] "
                "[--user-data-dir DIRECTORY] [--migrate-from OLD_DIRECTORY] [--portable|--system-user-data] "
                "[--omega-drive|--standard-drive] [--resolution <1x|2x|4x>]");
            return 2;
        }
    }
    const bool fixture_mode = native_save_fixture_mode || ship_interface_fixture_mode || goesnet_fixture_mode ||
                              persistence_fixture_mode || movie_fixture_mode || surface_fixture_mode ||
                              landing_fixture_mode || orbit_surface_fixture_mode || environment_fixture_mode ||
                              content_fixture_mode || oakenshield_fixture_mode;
    if (fixture_universe_seconds) {
        if (!fixture_mode) {
            noctis::log_event("error", "arguments", "--fixture-universe-seconds requires a fixture mode");
            return 2;
        }
        noctis::set_universe_seconds_override(fixture_universe_seconds);
    }
    if (fixture_mode && !user_data_override) {
        std::error_code error;
        user_data_override = std::filesystem::current_path(error);
        if (error) {
            noctis::log_event("error", "runtime_paths", error.message());
            return 1;
        }
    }
    if (fixture_mode && !migration_source)
        migration_source = user_data_override;
    if (migration_source) {
        std::error_code error;
        if (!std::filesystem::is_directory(*migration_source, error) || error) {
            noctis::log_event("error", "migration", "--migrate-from must name an accessible extracted game directory");
            return 2;
        }
    }
#ifdef __EMSCRIPTEN__
    // web/pre.js mounts browser storage (IndexedDB) here before main runs.
    if (!user_data_override) user_data_override = std::filesystem::path("/persistent");
#endif
    std::string path_error;
    if (!noctis::initialize_runtime_paths(argv[0], user_data_override, migration_source, &path_error,
                                          portable_mode_override)) {
        noctis::log_event("error", "runtime_paths", path_error);
        return 1;
    }
    if (!noctis::startup_report()) {
        return 1;
    }
    if (diagnostics_only) {
        if (migration_source) {
            noctis::log_event("warning", "migration",
                              "diagnostics are read-only; --migrate-from was inspected but not imported");
        }
        return 0;
    }
    const auto storage = noctis::prepare_runtime_storage(noctis::runtime_paths());
    if (!storage.ok) {
        noctis::log_event("error", "runtime_storage", storage.message);
        return 1;
    }
    if (reset_data_only) {
        std::string reset_error;
        if (!noctis::reset_runtime_storage(noctis::runtime_paths(), &reset_error)) {
            noctis::log_event("error", "runtime_storage", reset_error);
            return 1;
        }
        noctis::log_event("info", "runtime_storage", "user data reset to clean defaults");
        return 0;
    }
    configure_runtime_file_paths();
    if (export_starmap_path) {
        const auto starmap = noctis::runtime_paths().data_dir / "STARMAP.BIN";
        const auto guide   = noctis::runtime_paths().data_dir / "GUIDE.BIN";
        const auto rep     = noctis::export_starmap_packet(starmap, guide, *export_starmap_path);
        if (rep.status == noctis::StarmapExchangeStatus::no_records_to_export) {
            noctis::log_event("warning", "starmap_exchange", "no custom player records found to export");
            return 0;
        }
        if (rep.status != noctis::StarmapExchangeStatus::ok) {
            noctis::log_event("error", "starmap_exchange", rep.summary_message);
            return 1;
        }
        noctis::log_event("info", "starmap_exchange", rep.summary_message);
        return 0;
    }
    if (import_starmap_path) {
        const auto starmap = noctis::runtime_paths().data_dir / "STARMAP.BIN";
        const auto guide   = noctis::runtime_paths().data_dir / "GUIDE.BIN";
        noctis::StarmapImportOptions opts;
        opts.dry_run        = false;
        opts.skip_conflicts = true;
        const auto rep      = noctis::import_starmap_packet(starmap, guide, *import_starmap_path, opts);
        if (rep.status != noctis::StarmapExchangeStatus::ok) {
            noctis::log_event("error", "starmap_exchange", rep.summary_message);
            return 1;
        }
        noctis::log_event("info", "starmap_exchange", rep.summary_message);
        return 0;
    }
    if (validate_starmap_path) {
        const auto starmap = noctis::runtime_paths().data_dir / "STARMAP.BIN";
        const auto guide   = noctis::runtime_paths().data_dir / "GUIDE.BIN";
        noctis::StarmapImportOptions opts;
        opts.dry_run        = true;
        opts.skip_conflicts = true;
        const auto rep      = noctis::import_starmap_packet(starmap, guide, *validate_starmap_path, opts);
        if (rep.status != noctis::StarmapExchangeStatus::ok) {
            noctis::log_event("error", "starmap_exchange", rep.summary_message);
            return 1;
        }
        noctis::log_event("info", "starmap_exchange", rep.summary_message);
        return 0;
    }
    noctis::set_internal_resolution_change_callback(sync_internal_resolution_engine);
    noctis::load_display_settings(noctis::runtime_paths().config_dir);
    if (resolution_override) {
        noctis::set_internal_resolution_mode(*resolution_override);
    }
    sync_internal_resolution_engine(noctis::get_internal_resolution_mode());
    noctis::load_audio_settings(noctis::runtime_paths().config_dir);
    noctis::load_controls_settings(noctis::runtime_paths().config_dir);
    draw_hud        = noctis::get_setting_draw_hud();
    lens_flare_mode = noctis::get_setting_lens_flare_mode();
    seamless_border = noctis::get_setting_seamless_border();
    noctis::log_event("info", "runtime_storage",
                      "ready; copied=" + std::to_string(storage.copied_files) +
                          "; preserved=" + std::to_string(storage.preserved_files));
    if (prepare_user_data_only)
        return 0;
    if (!native_save_fixture_mode && !ship_interface_fixture_mode && !goesnet_fixture_mode &&
        !persistence_fixture_mode && !movie_fixture_mode && !surface_fixture_mode && !landing_fixture_mode &&
        !orbit_surface_fixture_mode && !environment_fixture_mode && !content_fixture_mode &&
        !oakenshield_fixture_mode) {
        noctis::log_event("info", "graphics", "initializing Raylib window 1280x720 (Noctis IV OM)");
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(1280, 720, "Noctis IV OM");
        if (!IsWindowReady()) {
            noctis::log_event("error", "graphics", "window initialization failed");
            return 1;
        }
        if (noctis::is_fullscreen()) {
            ToggleFullscreen();
        }
#ifdef __EMSCRIPTEN__
        // The canvas follows the browser window, which may be a phone or a
        // narrow pane; the viewport letterboxes the 320x200 frame inside it.
        SetWindowMinSize(160, 100);
#else
        SetWindowMinSize(640, 480);
        DisableCursor();
#endif
        auto image     = GenImageColor(adapted_width, adapted_height, {});
        screen_texture = LoadTextureFromImage(image);
        SetTextureFilter(screen_texture, TEXTURE_FILTER_POINT);
        UnloadImage(image);

        auto image_2x     = GenImageColor(adapted_width * 2, adapted_height * 2, {});
        screen_texture_2x = LoadTextureFromImage(image_2x);
        SetTextureFilter(screen_texture_2x, TEXTURE_FILTER_POINT);
        UnloadImage(image_2x);

        noctis::init_display_shaders();

        if (!no_audio_mode) {
            noctis::initialize_audio();
            std::atexit(noctis::shutdown_audio);
        }
        noctis::set_audio_toggle_handler([]() {
            noctis::toggle_audio_mute();
            status(noctis::is_audio_muted() ? "AUDIO MUTED" : "AUDIO ACTIVE", 50);
        });
        noctis::set_fullscreen_toggle_handler([]() {
            noctis::toggle_fullscreen();
            status(noctis::is_fullscreen() ? "FULLSCREEN" : "WINDOWED", 50);
            save_display_settings_current();
        });
        noctis::set_aspect_toggle_handler([]() {
            const auto next_mode = noctis::cycle_aspect_ratio_mode(noctis::get_aspect_ratio_mode());
            noctis::set_aspect_ratio_mode(next_mode);
            status(noctis::aspect_ratio_mode_name(next_mode), 50);
            save_display_settings_current();
        });
        noctis::set_upscale_toggle_handler([]() {
            const auto next_mode = noctis::cycle_upscale_mode(noctis::get_upscale_mode());
            noctis::set_upscale_mode(next_mode);
            status(noctis::upscale_mode_name(next_mode), 50);
            save_display_settings_current();
        });
        noctis::set_crt_toggle_handler([]() {
            const bool active = noctis::toggle_crt_shader();
            status(active ? "CRT SHADER: ACTIVE" : "CRT SHADER: DISABLED", 50);
            save_display_settings_current();
        });
        noctis::set_overlay_input_handler(noctis::gallery_viewer_input);
    }

    for (ir = 0; ir < 200; ir++) {
        m200[ir] = ir * 200;
    }

    n_offsets_map = (uint8_t *) malloc(om_bytes);
    n_globes_map  = (int8_t *) malloc((uint16_t) gl_bytes + (uint16_t) gl_brest);
    s_background  = (uint8_t *) malloc(st_bytes);
    p_background  = (uint8_t *) malloc(pl_bytes);
    /* NOTE: This is set to at least 65k because polymap keeps running over the
     * end. It happens in the original source too, and somehow isn't a problem
     * there, but we can't have it running over into random memory. The bug is
     * present in the original source.
     */
    p_surfacemap = (uint8_t *) malloc(ps_bytes | 65536);
    objectschart = (quadrant *) malloc(oc_bytes);
    ruinschart   = (uint8_t *) objectschart; // oc alias
    pvfile       = (uint8_t *) malloc(pv_bytes);
    adapted      = (uint8_t *) malloc(sc_bytes);
    txtr         = (uint8_t *) p_background;                             // txtr alias
    digimap2     = reinterpret_cast<uint8_t *>(&n_globes_map[gl_bytes]); // font alias

    if (pvfile && adapted && n_offsets_map && n_globes_map && p_background && s_background && p_surfacemap &&
        objectschart && lens_flares_init()) {
        lrv = loadpv(vehicle_handle, vehicle_ncc, 15, 15, 15, 0, 0, 0, 0, 1);

        if (lrv < 1) {
            noctis::log_event("error", "resource_load", "vehicle model could not be loaded from res/supports.nct");
            return 1;
        }

        load_QVRmaps();
        load_starface();
        load_digimap2();
    } else {
        noctis::log_event("error", "allocation", "runtime buffer or lens flare initialization failed");
        return 1;
    }

    noctis::log_event("info", "startup", "resources loaded; restoring runtime state");
    if (!unfreeze()) {
        return 1;
    }
    if (drive_override < 0)
        charge = -1;
    else if (drive_override > 0)
        noctis::restore_standard_drive(pwr, charge);
    if (movie_fixture_mode) {
        const auto &movie_root = noctis::runtime_paths().movies_dir;
        const auto file_hash   = [](const std::filesystem::path &path) {
            std::ifstream input(path, std::ios::binary);
            std::uint64_t hash = UINT64_C(14695981039346656037);
            char value;
            while (input.get(value)) {
                hash ^= static_cast<std::uint8_t>(value);
                hash *= UINT64_C(1099511628211);
            }
            return hash;
        };
        const auto fill_frame = [](std::uint8_t phase) {
            for (std::size_t index = 0; index < adapted_width * adapted_height; ++index) {
                adapted[index] = static_cast<std::uint8_t>(index + phase);
            }
        };
        for (std::size_t index = 0; index < 768; ++index)
            tmppal[index] = static_cast<std::uint8_t>(index % 64);

        movie_recorder.reset();
        handle_movie_extended_key(0x3D);
        std::memset(adapted, 0, adapted_width * adapted_height);
        draw_plus_overlay(false);
        const auto menu_hash = indexed_frame_hash();
        handle_movie_key('+', false);
        handle_movie_key('+', false);
        handle_movie_key('f', false);
        if (!handle_movie_key(13, false) || !movie_recorder.recording())
            return 1;
        bool black_flash_seen = false;
        for (std::uint8_t frame = 0; frame < 7; ++frame) {
            fill_frame(frame);
            advance_movie_capture(false);
            if (frame == 0) {
                black_flash_seen = std::all_of(adapted, adapted + adapted_width * adapted_height,
                                               [](std::uint8_t pixel) { return pixel == 0; });
            }
        }
        handle_movie_key('p', false);
        const auto paused_ticks = movie_recorder.elapsed_ticks();
        for (int frame = 0; frame < 5; ++frame)
            advance_movie_capture(false);
        handle_movie_key('p', false);
        handle_movie_key(13, false);
        if (!black_flash_seen || movie_recorder.captured_frames() != 3 ||
            movie_recorder.elapsed_ticks() != paused_ticks ||
            !std::filesystem::exists(movie_root / "001/00000001.BMP") ||
            !std::filesystem::exists(movie_root / "001/00000003.BMP") ||
            std::filesystem::exists(movie_root / "001/00000004.BMP"))
            return 1;

        std::filesystem::create_directories(movie_root / "002");
        std::ofstream(movie_root / "002/keep.txt") << "preserve";
        handle_movie_extended_key(0x3D);
        handle_movie_key('f', false);
        handle_movie_key(13, false);
        std::ifstream sentinel_input(movie_root / "002/keep.txt");
        std::string sentinel;
        sentinel_input >> sentinel;
        if (movie_recorder.recording() || sentinel != "preserve")
            return 1;
        handle_movie_extended_key(144);
        handle_movie_key(13, false);
        if (!movie_recorder.recording() || movie_recorder.deck() != 3)
            return 1;
        bool indicator_seen = false;
        for (std::uint8_t frame = 0; frame < 2; ++frame) {
            fill_frame(static_cast<std::uint8_t>(20 + frame));
            advance_movie_capture(false);
            if (frame == 0) {
                indicator_seen = std::all_of(adapted + 198 * adapted_width, adapted + adapted_width * adapted_height,
                                             [](std::uint8_t pixel) { return pixel == 127; });
            }
        }
        for (std::uint16_t frame = 0; frame < noctis::movie_ascent_cutoff_frames; ++frame) {
            fill_frame(static_cast<std::uint8_t>(30 + frame));
            advance_movie_capture(true);
        }
        std::size_t ascent_files = 0;
        for (const auto &entry : std::filesystem::directory_iterator(movie_root / "003")) {
            ascent_files += entry.is_regular_file();
        }
        for (std::uint32_t frame = 1; frame <= 34; ++frame) {
            char filename[16];
            std::snprintf(filename, sizeof(filename), "%08u.BMP", frame);
            if (!std::filesystem::exists(movie_root / "003" / filename))
                return 1;
        }
        if (!indicator_seen || movie_recorder.session_active() || movie_recorder.deck() != 4 || ascent_files != 34)
            return 1;
        std::printf("movie_fixture menu=%016llx space_first=%016llx surface_last=%016llx space_frames=3 pause=frozen "
                    "flashes=both occupied=preserved surface_frames=%zu ascent=cutoff\n",
                    static_cast<unsigned long long>(menu_hash),
                    static_cast<unsigned long long>(file_hash(movie_root / "001/00000001.BMP")),
                    static_cast<unsigned long long>(file_hash(movie_root / "003/00000034.BMP")), ascent_files);
        return 0;
    }
    if (persistence_fixture_mode) {
        const auto execute = [](const char *command) {
            std::strncpy(goesnet_command, command, sizeof(goesnet_command) - 1);
            goesnet_command[sizeof(goesnet_command) - 1] = 0;
            gnc_pos                                      = static_cast<int8_t>(std::strlen(goesnet_command) - 1);
            run_goesnet_module();
        };
        if (std::string_view(persistence_fixture_phase) == "advance") {
            execute("ST BALASTRACKONASTREYA_");
            if (ap_target_x != -18928 || ap_target_y != -29680 || ap_target_z != -67336) {
                noctis::log_event("error", "persistence_fixture", "remote target setup failed");
                return 1;
            }
            ap_reached = 1;
            execute("ST FELYSIA_");
            if (ip_targetted != 3) {
                noctis::log_event("error", "persistence_fixture", "local target setup failed");
                return 1;
            }
            autoscreenoff = revcontrols = menusalwayson = depolarize = 0;
            sys                                                      = 3;
            for (s_command = 1; s_command <= 4; ++s_command)
                pfs_commands();
            sys       = 2;
            dev_page  = 2;
            data      = 0;
            s_command = 4;
            dev_commands();
            last_snapshot     = 76'543'210;
            option_mouse_look = 2;
            roof_speed        = 1;
            freeze();
            std::printf("persistence_fixture phase=advance remote=balas local=felysia preferences=4 panel=3 omega=on "
                        "plus=restored\n");
            return 0;
        }
        if (std::string_view(persistence_fixture_phase) == "verify") {
            if (ap_target_x != -18928 || ap_target_y != -29680 || ap_target_z != -67336 || ip_targetted != 3 ||
                !autoscreenoff || !revcontrols || !menusalwayson || !depolarize || dev_page != 2 || data != 3 ||
                charge != -1 || last_snapshot != 76'543'210 || option_mouse_look != 2 || roof_speed != 1) {
                noctis::log_event("error", "persistence_fixture", "saved gameplay state was not restored");
                return 1;
            }
            freeze();
            noctis::NativeSaveState continued;
            const auto loaded = noctis::load_native_save(native_situation_file, continued);
            if (loaded.status != noctis::NativeSaveStatus::ok || continued.charge != -1 ||
                continued.last_snapshot != 76'543'210 || continued.option_mouse_look != 2 ||
                continued.roof_speed != 1) {
                noctis::log_event("error", "persistence_fixture", "continued state did not resave");
                return 1;
            }
            std::printf("persistence_fixture phase=verify remote=balas local=felysia preferences=4 panel=3 omega=on "
                        "plus=restored\n");
            return 0;
        }
        if (std::string_view(persistence_fixture_phase) == "deplete") {
            pwr    = 15000;
            charge = 0;
            freeze();
            std::printf("persistence_fixture phase=deplete power=15000 lithium=0 save=stored\n");
            return 0;
        }
        if (std::string_view(persistence_fixture_phase) == "standard") {
            if (pwr != 20000 || charge != 120) {
                noctis::log_event("error", "persistence_fixture", "standard drive did not recover depleted power");
                return 1;
            }
            freeze();
            noctis::NativeSaveState continued;
            const auto loaded = noctis::load_native_save(native_situation_file, continued);
            if (loaded.status != noctis::NativeSaveStatus::ok || continued.pwr != 20000 || continued.charge != 120) {
                noctis::log_event("error", "persistence_fixture", "standard drive recovery did not persist");
                return 1;
            }
            std::printf("persistence_fixture phase=standard power=20000 lithium=120 save=restored\n");
            return 0;
        }
        if (std::string_view(persistence_fixture_phase) == "clean-start") {
            if (pwr != 20000 || charge != 120 || secs < 1e8) {
                noctis::log_event("error", "persistence_fixture",
                                  "clean start was not initialized with full power and wall-clock time");
                return 1;
            }
            freeze();
            std::printf("persistence_fixture phase=clean-start power=20000 lithium=120 save=stored\n");
            return 0;
        }
        if (std::string_view(persistence_fixture_phase) == "clean-restart") {
            if (pwr != 20000 || charge != 120 || secs < 1e8) {
                noctis::log_event("error", "persistence_fixture", "clean restart depleted power or lithium");
                return 1;
            }
            freeze();
            std::printf("persistence_fixture phase=clean-restart power=20000 lithium=120 save=restored\n");
            return 0;
        }
        if (std::string_view(persistence_fixture_phase) == "legacy-unsynced") {
            if (pwr != 20000 || charge != 120 || secs < 1e8) {
                noctis::log_event("error", "persistence_fixture", "unsynchronized save depleted power or lithium");
                return 1;
            }
            freeze();
            std::printf("persistence_fixture phase=legacy-unsynced power=20000 lithium=120 save=restored\n");
            return 0;
        }
        noctis::log_event("error", "persistence_fixture", "unknown fixture phase");
        return 2;
    }
    if (goesnet_fixture_mode) {
        const auto execute = [](const char *command) {
            std::strncpy(goesnet_command, command, sizeof(goesnet_command) - 1);
            goesnet_command[sizeof(goesnet_command) - 1] = 0;
            gnc_pos                                      = static_cast<int8_t>(std::strlen(goesnet_command) - 1);
            run_goesnet_module();
        };
        execute("HELP_");
        if (goes_output_cells.find("PAR WHERE ST DL SL") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "native HELP did not reach the application output");
            return 1;
        }
        execute("WARP MIRACLE_");
        if (goes_output_cells.find("(UNKNOWN MODULE)") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "unknown command outcome mismatch");
            return 1;
        }
        execute("PAR_");
        if (goes_output_cells.find("INVALID") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "malformed command outcome mismatch");
            return 1;
        }
        execute("PAR F_");
        if (goes_output_cells.find("AMBIGUOUS SEARCH KEY") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "ambiguous lookup outcome mismatch");
            return 1;
        }
        execute("PAR DEFINITELY_NOT_FOUND_");
        if (goes_output_cells.find("OBJECT NOT FOUND") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "not-found lookup outcome mismatch");
            return 1;
        }
        const double saved_dzat_x = dzat_x, saved_dzat_y = dzat_y, saved_dzat_z = dzat_z;
        dzat_x = 3797120;
        dzat_y = -4352112;
        dzat_z = -925018;
        execute("PAR MIRACLE_");
        if (goes_output_cells.find("NAME: MIRACLE") == std::string::npos ||
            goes_output_cells.find("X=3979984") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "DOS-confirmed MIRACLE text fields mismatch");
            return 1;
        }
        dzat_x = saved_dzat_x;
        dzat_y = saved_dzat_y;
        dzat_z = saved_dzat_z;
        execute("PAR FELYSIA_");
        if (goes_output_cells.find("NAME: FELYSIA") == std::string::npos ||
            goes_output_cells.find("X=-18928") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "native PAR did not reach the application output");
            return 1;
        }
        execute("DL FELYSIA_");
        if (goes_output_cells.find("FELYSIA") == std::string::npos ||
            goes_output_cells.find("(238 NOTES)") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "P15 planet note count mismatch");
            return 1;
        }
        execute("ST BALASTRACKONASTREYA_");
        if (ap_target_x != -18928 || ap_target_y != -29680 || ap_target_z != -67336 || !ap_targetted) {
            noctis::log_event("error", "goesnet_fixture", "native ST did not apply its typed target action");
            return 1;
        }
        ap_reached = 1;
        execute("ST FELYSIA_");
        if (ip_targetted != 3) {
            noctis::log_event("error", "goesnet_fixture", "native local ST did not apply its typed target action");
            return 1;
        }

        // Drive normal cartography commands, not a fixture-only label writer.
        execute("ST MIRACLE_");
        ap_targetted     = 1;
        ap_targetting    = 0;
        labplanet        = 0;
        ap_target_previd = 12345;
        update_star_label();
        star_label_pos = -1;
        dev_page       = 3;
        s_command      = 1;
        dev_commands();
        std::memcpy(star_label, "WORKFLOW STAR", 13);
        labstar_char = 13;
        dev_commands();
        const auto workflow_star_pos = star_label_pos;
        execute("PAR WORKFLOW_STAR_");
        if (workflow_star_pos < sm_consolidated || goes_output_cells.find("NAME: WORKFLOW STAR") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "normal star label did not resolve through PAR");
            return 1;
        }
        execute("SL_");
        if (goes_output_cells.find("WORKFLOW STAR") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "normal star label did not resolve through SL");
            return 1;
        }

        ip_targetted   = 0;
        labstar        = 0;
        prev_planet_id = 12345;
        update_planet_label();
        planet_label_pos = -1;
        dev_page         = 3;
        s_command        = 2;
        dev_commands();
        std::memcpy(planet_label, "WORKFLOW PLANET", 15);
        labplanet_char = 15;
        dev_commands();
        const auto workflow_planet_pos = planet_label_pos;
        execute("PAR WORKFLOW_PLANET_");
        if (workflow_planet_pos < sm_consolidated ||
            goes_output_cells.find("NAME: WORKFLOW PLANET") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "normal planet label did not resolve through PAR");
            return 1;
        }
        execute("WHERE WORKFLOW_PLANET_");
        if (goes_output_cells.find("BALASTRACKONASTREYA") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "normal planet label did not resolve through WHERE");
            return 1;
        }
        ap_reached = 1;
        execute("ST WORKFLOW_PLANET_");
        if (ip_targetted != 0) {
            noctis::log_event("error", "goesnet_fixture", "normal planet label did not resolve through ST");
            return 1;
        }

        execute("CAST FELYSIA:WORKFLOW GUIDE NOTE_");
        execute("CAT FELYSIA:239..239_");
        if (goes_output_cells.find("WORKFLOW GUIDE NOTE") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "catalog append did not survive reopen");
            return 1;
        }
        execute("REP FELYSIA:239:WORKFLOW REPLACED_");
        execute("CAT FELYSIA:239..239_");
        if (goes_output_cells.find("WORKFLOW REPLACED") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "catalog replacement did not survive reopen");
            return 1;
        }
        execute("REP FELYSIA:1:PROTECTED CHANGE_");
        if (goes_output_cells.find("REQUEST REJECTED") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "protected catalog replacement was accepted");
            return 1;
        }
        execute("DELE FELYSIA:239_");
        execute("CAT FELYSIA:239..239_");
        if (goes_output_cells.find("THERE WERE NO RECORDS") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "catalog deletion did not survive reopen");
            return 1;
        }

        star_label_pos = workflow_star_pos;
        labstar        = 0;
        labplanet      = 0;
        ap_targetted   = 1;
        ap_targetting  = 0;
        dev_page       = 3;
        s_command      = 1;
        dev_commands();
        planet_label_pos = workflow_planet_pos;
        labplanet        = 0;
        labstar          = 0;
        ip_targetted     = 0;
        s_command        = 2;
        dev_commands();
        execute("PAR WORKFLOW_STAR_");
        if (goes_output_cells.find("OBJECT NOT FOUND") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "removed star label remained resolvable");
            return 1;
        }
        execute("PAR WORKFLOW_PLANET_");
        if (goes_output_cells.find("OBJECT NOT FOUND") == std::string::npos) {
            noctis::log_event("error", "goesnet_fixture", "removed planet label remained resolvable");
            return 1;
        }
        remove(situation_file);
        freeze();
        FILE *legacy = fopen(situation_file, "rb");
        if (legacy != nullptr) {
            fclose(legacy);
            noctis::log_event("error", "goesnet_fixture", "native save still emitted current.bin");
            return 1;
        }
        for (const char *name : {"comm.bin", "GOESfile.txt"}) {
            const auto obsolete = noctis::runtime_paths().data_dir / name;
            FILE *file          = fopen(obsolete.string().c_str(), "rb");
            if (file != nullptr) {
                fclose(file);
                noctis::log_event("error", "goesnet_fixture", "legacy GOESnet interchange file was created");
                return 1;
            }
        }
        std::printf("goesnet_fixture help=p14 dl=p15 data=p22 target=remote+local labels=roundtrip catalog=roundtrip "
                    "shell=absent interchange=absent legacy_save=absent\n");
        return 0;
    }
    if (ship_interface_fixture_mode) {
        const auto all_commands_present = [] {
            for (int slot = 0; slot < 4; ++slot) {
                if (ctb[20 + 27 * slot] == 0)
                    return false;
            }
            return true;
        };

        autoscreenoff = revcontrols = menusalwayson = depolarize = 0;
        clear_onboard_screen();
        prefs();
        if (!all_commands_present() || std::string_view(&ctb[20]) != "auto screen sleep off" ||
            std::string_view(&ctb[20 + 27]) != "normal pitch controls") {
            noctis::log_event("error", "ship_interface_fixture", "preference menu did not render its disabled state");
            return 1;
        }
        for (int command_index = 1; command_index <= 4; ++command_index) {
            s_command = command_index;
            pfs_commands();
        }
        clear_onboard_screen();
        prefs();
        if (!autoscreenoff || !revcontrols || !menusalwayson || !depolarize || !all_commands_present() ||
            std::string_view(&ctb[20]) != "auto screen sleep on" ||
            std::string_view(&ctb[20 + 27]) != "reverse pitch controls") {
            noctis::log_event("error", "ship_interface_fixture",
                              "preference commands did not render their enabled state");
            return 1;
        }

        clear_onboard_screen();
        fcs();
        if (!all_commands_present()) {
            noctis::log_event("error", "ship_interface_fixture", "flight-control menu has a missing command");
            return 1;
        }
        for (int page = 0; page <= 4; ++page) {
            dev_page = page;
            clear_onboard_screen();
            devices();
            if (!all_commands_present()) {
                noctis::log_event("error", "ship_interface_fixture", "device menu has a missing command");
                return 1;
            }
        }

        dev_page = 2;
        data     = 0;
        for (int panel = 1; panel <= 3; ++panel) {
            s_command = panel + 1;
            dev_commands();
            if (data != panel) {
                noctis::log_event("error", "ship_interface_fixture", "HUD data-panel route failed");
                return 1;
            }
        }

        draw_hud        = 0;
        lens_flare_mode = -1;
        seamless_border = 1;
        status("MOUSELOOK ENABLED", 100);
        if (std::string_view(reinterpret_cast<char *>(fcs_status)) != "MOUSELOOK " ||
            std::string_view(reinterpret_cast<char *>(fcs_status_extended)) != "MOUSELOOK ENABLED") {
            noctis::log_event("error", "ship_interface_fixture", "extended surface status was not retained");
            return 1;
        }
        std::memset(adapted, 0, adapted_width * adapted_height);
        about = 1;
        draw_plus_overlay(false);
        const auto help_hash    = indexed_frame_hash();
        const bool help_nonzero = std::any_of(adapted, adapted + adapted_width * adapted_height,
                                              [](std::uint8_t pixel) { return pixel != 0; });
        about                   = 0;
        graphics_menu_status    = 1;
        draw_plus_overlay(false);
        const auto menu_hash = indexed_frame_hash();
        graphics_menu_status = 0;
        if (!help_nonzero || help_hash == menu_hash) {
            noctis::log_event("error", "ship_interface_fixture", "F1/F2 overlays did not render distinctly");
            return 1;
        }

        const auto save_result = noctis::save_native_save(native_situation_file, capture_native_state());
        noctis::NativeSaveState reloaded;
        const auto load_result = noctis::load_native_save(native_situation_file, reloaded);
        if (save_result.status != noctis::NativeSaveStatus::ok || load_result.status != noctis::NativeSaveStatus::ok ||
            !reloaded.autoscreenoff || !reloaded.revcontrols || !reloaded.menusalwayson || !reloaded.depolarize ||
            reloaded.draw_hud != 0 || reloaded.lens_flare_mode != -1 || reloaded.seamless_border != 1) {
            noctis::log_event("error", "ship_interface_fixture", "preferences did not survive native persistence");
            return 1;
        }
        std::printf("ship_interface_fixture screens=3 menus=4 device_pages=5 hud_panels=3 preferences=7 overlays=2 "
                    "status=extended save=restored\n");
        return 0;
    }
    if (native_save_fixture_mode) {
        const auto expected = noctis::encode_native_save(capture_native_state());
        freeze();
        noctis::NativeSaveState reloaded;
        const auto load_result = noctis::load_native_save(native_situation_file, reloaded);
        if (load_result.status != noctis::NativeSaveStatus::ok || noctis::encode_native_save(reloaded) != expected) {
            noctis::log_event("error", "native_save_fixture", "application state did not round-trip exactly");
            return 1;
        }
        noctis::SurfaceRestore restored_surface;
        const auto surface_result =
            noctis::load_or_migrate_surface(native_surface_file, surface_file, restored_surface);
        if (surface_result.status != noctis::NativeSaveStatus::ok &&
            surface_result.status != noctis::NativeSaveStatus::not_found) {
            noctis::log_event("error", "surface_save", surface_result.message);
            return 1;
        }
        if (surface_result.status == noctis::NativeSaveStatus::ok) {
            const auto surface_bytes = noctis::encode_surface_save(restored_surface.state);
            const std::string surface_label =
                restored_surface.migrated
                    ? "migrated-" + std::string(noctis::legacy_layout_name(restored_surface.legacy_layout))
                    : "native-v1";
            printf("native_save_fixture version=%u bytes=%zu state=restored surface=%s bytes=%zu\n",
                   noctis::native_save_version, expected.size(), surface_label.c_str(), surface_bytes.size());
        } else {
            printf("native_save_fixture version=%u bytes=%zu state=restored surface=absent\n",
                   noctis::native_save_version, expected.size());
        }
        return 0;
    }
    memset(adapted, 0, QUADWORDS * 4);
    QUADWORDS -= 1440 * internal_res_scale * internal_res_scale;
    pqw = QUADWORDS;
    if (!surface_fixture_mode && !landing_fixture_mode && !orbit_surface_fixture_mode && !environment_fixture_mode &&
        !content_fixture_mode && !oakenshield_fixture_mode) {
        handle_input();
    }
    mpul = 0;
    dpp  = 210.0f * internal_res_scale;
    change_camera_lens();
    //   0..64  Vehicle, computer selections, artifacts. Cobalt Blue, depending
    //   on the color from the star.
    //  64..128 cosmos, galactic background, clear skies and "suplucsi effect".
    //  from the white electric blue.
    // 128..192 Stars (Continuous cyclic shaders) or moons (non-constant)
    // 192..256 Planets (Non constant)
    tavola_colori(range8088, 0, 64, 16, 32, 63);
    tavola_colori(tmppal, 0, 256, 64, 64, 64);
    // causa il recupero dell'eventuale contenuto dello schermo
    // di output della GOES command net
    force_update = 1;
    if (surface_fixture_mode) {
        const auto *fixture = find_surface_fixture(surface_fixture_name);
        if (fixture == nullptr) {
            noctis::log_event("error", "surface_fixture", "unknown surface fixture case");
            return 2;
        }
        if (nearstar_x != -18928.0 || nearstar_y != -29680.0 || nearstar_z != -67336.0) {
            noctis::log_event("error", "surface_fixture", "expected seeded FELYSIA current.bin");
            return 1;
        }
        select_surface_fixture(*fixture);
        if (nearstar_class != fixture->star_class || nearstar_p_type[ip_targetted] != fixture->body_type) {
            noctis::log_event("error", "surface_fixture", "generated body does not match fixture case");
            return 1;
        }
        reset_simulation_time(0);
        planet_xyz(ip_targetted);
        dzat_x = plx;
        dzat_y = ply;
        dzat_z = plz;
        dxx    = dzat_x - nearstar_x;
        dyy    = dzat_y - nearstar_y;
        dzz    = dzat_z - nearstar_z;
        dsd    = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
        proj_from_vehicle();
        landing_point = 1;
        draw_planets();
        landing_point = 0;
        entryflag     = 0;
        planetary_main();
        return 0;
    }
    if (oakenshield_fixture_mode) {
        ap_target_x     = 3321776.0;
        ap_target_y     = -4323134.0;
        ap_target_z     = -1004416.0;
        ap_target_class = 5;
        ap_target_ray   = 1.086F;
        ap_target_spin  = 0;
        ap_target_r     = 63;
        ap_target_g     = 58;
        ap_target_b     = 40;
        _delay          = 0;
        prepare_nearstar();
        ip_targetted = 0;

        landing_pt_lon = 0;
        landing_pt_lat = 60;
        reset_simulation_time(0);
        planet_xyz(ip_targetted);
        dzat_x = plx;
        dzat_y = ply;
        dzat_z = plz;
        dxx    = dzat_x - nearstar_x;
        dyy    = dzat_y - nearstar_y;
        dzz    = dzat_z - nearstar_z;
        dsd    = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
        proj_from_vehicle();
        landing_point = 1;
        draw_planets();
        landing_point = 0;
        entryflag     = 0;

        // Part 1: Descent, touchdown, and ESC surface save
        static std::uint32_t oak_touchdown_frame = 0;
        static int oak_frame_count               = 0;
        oak_touchdown_frame                      = 0;
        oak_frame_count                          = 0;
        noctis::reset_input_state();
        noctis::set_input_provider([]() {
            noctis::InputFrame frame;
            ++oak_frame_count;
            if (landed && oak_touchdown_frame == 0) {
                oak_touchdown_frame = oak_frame_count;
            }
            if (landed && oak_frame_count > static_cast<int>(oak_touchdown_frame) + 5) {
                frame.escape_down = true;
            }
            return frame;
        });
        planetary_main();
        noctis::reset_input_provider();

        // Verify starfield and terrain palette on dim star after create_sky has run
        const bool sky_stars_visible = surface_palette[127 * 3 + 0] >= 40;
        const bool terrain_defined   = surface_palette[0] >= 0 && surface_palette[44 * 3 + 0] >= 4;
        const bool landing_ok        = exitflag == 1 && landed && oak_touchdown_frame > 0 && oak_touchdown_frame < 550;

        // Part 2: Surface resume with corrupted ip_targetted (simulating power loss)
        noctis::SurfaceRestore surface_restore;
        const auto surface_result = noctis::load_or_migrate_surface(native_surface_file, surface_file, surface_restore);
        bool resume_ok            = false;
        bool oak_resume_palette_valid = false;
        static int oak_resume_count   = 0;
        oak_resume_count              = 0;
        if (surface_result.status == noctis::NativeSaveStatus::ok) {
            landing_pt_lon = surface_restore.state.landing_longitude;
            landing_pt_lat = surface_restore.state.landing_latitude;
            // Force ip_targetted to -1 to verify defensive recovery
            ip_targetted = -1;
            if (ip_targetted < 0 || ip_targetted >= nearstar_nob) {
                int16_t best_body = 0;
                double min_d2     = -1.0;
                for (int16_t n = 0; n < nearstar_nob; ++n) {
                    planet_xyz(n);
                    const double dpx = plx - dzat_x;
                    const double dpy = ply - dzat_y;
                    const double dpz = plz - dzat_z;
                    const double d2  = dpx * dpx + dpy * dpy + dpz * dpz;
                    if (min_d2 < 0.0 || d2 < min_d2) {
                        min_d2    = d2;
                        best_body = n;
                    }
                }
                ip_targetted = best_body;
            }
            update_star_label();
            update_planet_label();
            getsecs();
            planet_xyz(ip_targetted);
            dzat_x = plx;
            dzat_y = ply;
            dzat_z = plz;
            dxx    = dzat_x - nearstar_x;
            dyy    = dzat_y - nearstar_y;
            dzz    = dzat_z - nearstar_z;
            dsd    = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
            proj_from_vehicle();
            landing_point = 1;
            draw_planets();
            landing_point = 0;
            entryflag     = 1;

            noctis::reset_input_state();
            noctis::set_input_provider([]() {
                noctis::InputFrame frame;
                ++oak_resume_count;
                if (oak_resume_count >= 5) {
                    frame.escape_down = true;
                }
                return frame;
            });
            planetary_main();
            noctis::reset_input_provider();
            resume_ok                = (exitflag == 1 && landed && ip_targetted == 0);
            oak_resume_palette_valid = (surface_palette[127 * 3 + 0] >= 40 && surface_palette[44 * 3 + 0] >= 4);
        }

        // Part 3: Test ESC during descent cleanly aborts to ship
        planet_xyz(ip_targetted);
        dzat_x = plx;
        dzat_y = ply;
        dzat_z = plz;
        dxx    = dzat_x - nearstar_x;
        dyy    = dzat_y - nearstar_y;
        dzz    = dzat_z - nearstar_z;
        dsd    = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
        proj_from_vehicle();
        landing_point = 1;
        draw_planets();
        landing_point                       = 0;
        entryflag                           = 0;
        landed                              = 0;
        static int oak_descent_abort_frames = 0;
        oak_descent_abort_frames            = 0;
        noctis::reset_input_state();
        noctis::set_input_provider([]() {
            noctis::InputFrame frame;
            ++oak_descent_abort_frames;
            if (oak_descent_abort_frames >= 10) {
                frame.escape_down = true;
            }
            return frame;
        });
        planetary_main();
        noctis::reset_input_provider();
        const bool abort_ok = (exitflag == 0 && !landed && oak_descent_abort_frames == 10);

        if (!landing_ok || !sky_stars_visible || !terrain_defined || !resume_ok || !oak_resume_palette_valid ||
            !abort_ok) {
            fprintf(stderr,
                    "oakenshield_fixture failed: landing_ok=%d (touchdown=%u) sky_stars=%d terrain=%d resume_ok=%d "
                    "resume_palette=%d abort_ok=%d\n",
                    landing_ok, oak_touchdown_frame, sky_stars_visible, terrain_defined, resume_ok,
                    oak_resume_palette_valid, abort_ok);
            return 1;
        }

        printf("oakenshield_fixture touchdown=%u sky_stars=ok terrain=ok resume=ok palette=valid abort=ok status=ok\n",
               oak_touchdown_frame);
        return 0;
    }
    if (landing_fixture_mode || orbit_surface_fixture_mode) {
        const auto *fixture = find_surface_fixture("felysia-habitable");
        if (nearstar_x != -18928.0 || nearstar_y != -29680.0 || nearstar_z != -67336.0 || fixture == nullptr) {
            noctis::log_event("error", "landing_fixture", "expected seeded FELYSIA current.bin");
            return 1;
        }
        select_surface_fixture(*fixture);
        reset_simulation_time(0);
        planet_xyz(ip_targetted);
        dzat_x = plx;
        dzat_y = ply;
        dzat_z = plz;
        dxx    = dzat_x - nearstar_x;
        dyy    = dzat_y - nearstar_y;
        dzz    = dzat_z - nearstar_z;
        dsd    = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
        proj_from_vehicle();

        ip_reaching   = 0;
        ip_reached    = 1;
        landing_point = 0;
        s_command     = 4;
        fcs_commands();
        if (!landing_point || landing_pt_lon != 0 || landing_pt_lat != 60) {
            noctis::log_event("error", "landing_fixture", "FCS did not enter landing selection");
            return 1;
        }
        landing_pt_lon = fixture->longitude;
        landing_pt_lat = fixture->latitude;
        draw_planets();
        landing_point = 0;
        if (orbit_surface_fixture_mode) {
            journey_orbit_hash = indexed_frame_hash();
        }
        const double ship_x = dzat_x;
        const double ship_y = dzat_y;
        const double ship_z = dzat_z;

        noctis::reset_input_state();
        noctis::set_input_provider(scripted_landing_input);
        entryflag = 0;
        planetary_main();
        noctis::reset_input_provider();

        const bool returned_normally = exitflag == 0 && !landed && landing_fixture_return_frame > 0 &&
                                       landing_fixture_frames < 10000 && dzat_x == ship_x && dzat_y == ship_y &&
                                       dzat_z == ship_z && pos_x == 0 && pos_y == 0 && pos_z == -3100;
        if (!returned_normally) {
            fprintf(stderr,
                    "landing_fixture_debug exit=%d landed=%d touchdown=%u returned=%u frames=%u pos=%.3f,%.3f,%.3f "
                    "ship=%.17g,%.17g,%.17g expected=%.17g,%.17g,%.17g\n",
                    exitflag, landed, landing_fixture_touchdown_frame, landing_fixture_return_frame,
                    landing_fixture_frames, pos_x, pos_y, pos_z, dzat_x, dzat_y, dzat_z, ship_x, ship_y, ship_z);
            noctis::log_event("error", "landing_fixture", "surface capsule did not return cleanly to the ship");
            return 1;
        }
        if (orbit_surface_fixture_mode) {
            if (journey_orbit_hash == 0 || journey_touchdown_hash == 0 || journey_outbound_hash == 0 ||
                journey_return_hash == 0 || fixture_tree_draws == 0 || fixture_animal_draws == 0 ||
                fixture_ruin_draws == 0 || fixture_capsule_draws == 0) {
                fprintf(stderr,
                        "orbit_surface_debug orbit=%016llx ground=%016llx outbound=%016llx capsule=%016llx trees=%u "
                        "animals=%u ruins=%u capsule_draws=%u rendered=%u\n",
                        static_cast<unsigned long long>(journey_orbit_hash),
                        static_cast<unsigned long long>(journey_touchdown_hash),
                        static_cast<unsigned long long>(journey_outbound_hash),
                        static_cast<unsigned long long>(journey_return_hash), fixture_tree_draws, fixture_animal_draws,
                        fixture_ruin_draws, fixture_capsule_draws, journey_rendered_frames);
                noctis::log_event("error", "orbit_surface_fixture",
                                  "journey missed a required visual/content checkpoint");
                return 1;
            }
            printf("orbit_surface_fixture longitude=%d latitude=%d touchdown=%u returned=%u frames=%u rendered=%u "
                   "orbit=%016llx ground=%016llx outbound=%016llx capsule=%016llx trees=%u animals=%u ruins=%u "
                   "capsule_draws=%u ship_position=restored\n",
                   landing_pt_lon, landing_pt_lat, landing_fixture_touchdown_frame, landing_fixture_return_frame,
                   landing_fixture_frames, journey_rendered_frames, static_cast<unsigned long long>(journey_orbit_hash),
                   static_cast<unsigned long long>(journey_touchdown_hash),
                   static_cast<unsigned long long>(journey_outbound_hash),
                   static_cast<unsigned long long>(journey_return_hash), fixture_tree_draws, fixture_animal_draws,
                   fixture_ruin_draws, fixture_capsule_draws);
            return 0;
        }
        printf("landing_fixture request=1 longitude=%d latitude=%d touchdown=%u returned=%u frames=%u "
               "ship_position=restored\n",
               landing_pt_lon, landing_pt_lat, landing_fixture_touchdown_frame, landing_fixture_return_frame,
               landing_fixture_frames);
        return 0;
    }
    if (environment_fixture_mode || content_fixture_mode) {
        const auto *fixture = find_surface_fixture(environment_fixture_name);
        if (nearstar_x != -18928.0 || nearstar_y != -29680.0 || nearstar_z != -67336.0 || fixture == nullptr) {
            noctis::log_event("error", "environment_fixture", "expected seeded FELYSIA current.bin");
            return 1;
        }
        select_surface_fixture(*fixture);
        landing_pt_lon = environment_fixture_longitude;
        landing_pt_lat = environment_fixture_latitude;
        reset_simulation_time(0);
        planet_xyz(ip_targetted);
        dzat_x = plx;
        dzat_y = ply;
        dzat_z = plz;
        dxx    = dzat_x - nearstar_x;
        dyy    = dzat_y - nearstar_y;
        dzz    = dzat_z - nearstar_z;
        dsd    = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
        proj_from_vehicle();
        landing_point = 1;
        draw_planets();
        landing_point = 0;
        noctis::reset_input_state();
        noctis::set_input_provider(scripted_surface_frame_input);
        entryflag = 0;
        planetary_main();
        noctis::reset_input_provider();
        const std::uint32_t expected_frames = content_fixture_mode ? 3 : 1;
        if (!exitflag || environment_fixture_frames != expected_frames) {
            noctis::log_event("error", "environment_fixture", "live environment frame did not complete");
            return 1;
        }
        if (content_fixture_mode) {
            printf("content_fixture case=%s longitude=%d latitude=%d frames=%u trees=%u rocks=%u animals=%u ruins=%u "
                   "capsule=%u frame=%016llx\n",
                   environment_fixture_name, landing_pt_lon, landing_pt_lat, environment_fixture_frames,
                   fixture_tree_draws, fixture_rock_draws, fixture_animal_draws, fixture_ruin_draws,
                   fixture_capsule_draws, static_cast<unsigned long long>(environment_fixture_hash));
            return 0;
        }
        printf(
            "environment_fixture case=%s longitude=%d latitude=%d scenario=%d rainy=%.3f waves=%d/%d frame=%016llx\n",
            environment_fixture_name, landing_pt_lon, landing_pt_lat, sctype, rainy, waves_in, waves_out,
            static_cast<unsigned long long>(environment_fixture_hash));
        return 0;
    }
    // recupero della situazione di superficie
    noctis::SurfaceRestore surface_restore;
    const auto surface_result = noctis::load_or_migrate_surface(native_surface_file, surface_file, surface_restore);
    if (surface_result.status != noctis::NativeSaveStatus::ok &&
        surface_result.status != noctis::NativeSaveStatus::not_found) {
        noctis::log_event("error", "surface_save", surface_result.message);
        return 1;
    }
    if (surface_result.status == noctis::NativeSaveStatus::ok) {
        if (nearstar_nob <= 0) {
            noctis::log_event("warn", "surface_save",
                              "ignoring orphaned surface save: current system has no celestial bodies");
            std::filesystem::remove(native_surface_file);
            std::filesystem::remove(surface_file);
        } else {
            landing_pt_lon = surface_restore.state.landing_longitude;
            landing_pt_lat = surface_restore.state.landing_latitude;
            if (surface_restore.migrated) {
                noctis::log_event("info", "legacy_migration",
                                  std::string("migrated ") +
                                      std::string(noctis::legacy_layout_name(surface_restore.legacy_layout)) +
                                      " surface checkpoint to native v1");
            }
            // recupero labels del pianeta e della stella-bersaglio
            if (ip_targetted < 0 || ip_targetted >= nearstar_nob) {
                int16_t best_body = 0;
                double min_d2     = -1.0;
                for (int16_t n = 0; n < nearstar_nob; ++n) {
                    planet_xyz(n);
                    const double dpx = plx - dzat_x;
                    const double dpy = ply - dzat_y;
                    const double dpz = plz - dzat_z;
                    const double d2  = dpx * dpx + dpy * dpy + dpz * dpz;
                    if (min_d2 < 0.0 || d2 < min_d2) {
                        min_d2    = d2;
                        best_body = n;
                    }
                }
                ip_targetted = best_body;
            }
            update_star_label();
            update_planet_label();
            // risincronizzazione istantanea della posizione della navicella
            getsecs();
            planet_xyz(ip_targetted);
            dzat_x = plx;
            dzat_y = ply;
            dzat_z = plz;
            // calcolo della distanza dalla stella primaria
            dxx = dzat_x - nearstar_x;
            dyy = dzat_y - nearstar_y;
            dzz = dzat_z - nearstar_z;
            dsd = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
            // rielaborazione superficie planetaria
            proj_from_vehicle();
            landing_point = 1;
            draw_planets();
            landing_point = 0;
            // ripresa del ciclo di esplorazione planetaria
            entryflag = 1;
            planetary_main();
            // termine esplorazione
            opencapcount = 86;
            opencapdelta = -2;
            holdtomiddle = 1;
            pp_gravity   = 1;
            QUADWORDS    = (adapted_width * adapted_height) / 4;
            memset(adapted, 0, adapted_width * adapted_height);
            QUADWORDS = pqw;

            if (exitflag) {
                freeze();
                return 0;
            }
        }
    }

    // The browser build compiles with ASYNCIFY, so this loop (and the nested
    // surface loop in planetary_main) yields to the browser in swapBuffers.
    // A browser session never ends from Escape (it also releases pointer lock
    // and fullscreen); it autosaves instead and the tab can simply be closed.
#ifdef __EMSCRIPTEN__
    constexpr bool session_can_end = false;
#else
    constexpr bool session_can_end = true;
#endif
    do {
        loop();
        if (graphical_smoke_mode && ++graphical_smoke_frames >= 3) {
            mc          = 27;
            stspeed     = 0;
            ip_reaching = 0;
            lifter      = 0;
        }
    } while (!session_can_end || (mc != 27) || stspeed || ip_reaching || lifter);
    remove(surface_file);
    remove(native_surface_file);

    freeze();
    noctis::shutdown_audio();
    if (graphical_smoke_mode) {
        noctis::log_event("info", "graphical_smoke", "window opened, resources loaded, and three frames presented");
        noctis::shutdown_gallery_viewer();
        UnloadTexture(screen_texture);
        UnloadTexture(screen_texture_2x);
        noctis::cleanup_display_shaders();
        CloseWindow();
    }
}

void swapBuffers() {
    if (orbit_surface_fixture_mode) {
        const auto hash = indexed_frame_hash();
        ++journey_rendered_frames;
        if (landing_fixture_touchdown_frame != 0 && journey_touchdown_hash == 0) {
            journey_touchdown_hash = hash;
        }
        if (landing_fixture_phase == LandingFixturePhase::walking_back && landing_fixture_return_frame == 0 &&
            journey_outbound_hash == 0) {
            journey_outbound_hash = hash;
        }
        if (landing_fixture_return_frame != 0 && journey_return_hash == 0) {
            journey_return_hash = hash;
        }
        return;
    }
    if (environment_fixture_mode || content_fixture_mode) {
        if (landed) {
            environment_fixture_hash = indexed_frame_hash();
            ++environment_fixture_frames;
        }
        return;
    }
    if (surface_fixture_mode || landing_fixture_mode || oakenshield_fixture_mode) {
        return;
    }
    BeginDrawing();
    ClearBackground(BLACK);

    static std::vector<std::uint8_t> pixels(adapted_width * adapted_height * 4);
    if (screen_texture.width != adapted_width || screen_texture.height != adapted_height) {
        if (IsTextureValid(screen_texture)) {
            UnloadTexture(screen_texture);
        }
        if (IsTextureValid(screen_texture_2x)) {
            UnloadTexture(screen_texture_2x);
        }
        auto image     = GenImageColor(adapted_width, adapted_height, {});
        screen_texture = LoadTextureFromImage(image);
        SetTextureFilter(screen_texture, TEXTURE_FILTER_POINT);
        UnloadImage(image);

        auto image_2x     = GenImageColor(adapted_width * 2, adapted_height * 2, {});
        screen_texture_2x = LoadTextureFromImage(image_2x);
        SetTextureFilter(screen_texture_2x, TEXTURE_FILTER_POINT);
        UnloadImage(image_2x);

        pixels.resize(adapted_width * adapted_height * 4);
    }
    if (pixels.size() != static_cast<std::size_t>(adapted_width * adapted_height * 4)) {
        pixels.resize(adapted_width * adapted_height * 4);
    }
    noctis::expand_indexed_rgba(adapted, adapted_width * adapted_height, currpal, pixels.data());
    if (suit_torch) {
        noctis::apply_suit_torch_rgba(pixels.data(), adapted, adapted_width, adapted_height);
    }
    UpdateTexture(screen_texture, pixels.data());

    int render_w = GetRenderWidth();
    int render_h = GetRenderHeight();
    if (render_w <= 0 || render_h <= 0) {
        render_w = GetScreenWidth();
        render_h = GetScreenHeight();
    }
    const auto viewport = noctis::calculate_viewport(render_w, render_h, noctis::get_aspect_ratio_mode());

    const auto upscale_mode = noctis::get_upscale_mode();
    noctis::begin_crt_shader(render_w, render_h);

    if (upscale_mode == noctis::UpscaleMode::edge_scale2x) {
        static std::vector<std::uint32_t> pixels_2x((adapted_width * 2) * (adapted_height * 2));
        if (pixels_2x.size() != static_cast<std::size_t>((adapted_width * 2) * (adapted_height * 2))) {
            pixels_2x.resize((adapted_width * 2) * (adapted_height * 2));
        }
        noctis::scale2x_rgba(reinterpret_cast<const std::uint32_t *>(pixels.data()),
                             adapted_width, adapted_height, pixels_2x.data());
        UpdateTexture(screen_texture_2x, pixels_2x.data());
        DrawTexturePro(screen_texture_2x,
                       Rectangle{0.0f, 0.0f, static_cast<float>(adapted_width * 2), static_cast<float>(adapted_height * 2)},
                       Rectangle{viewport.x, viewport.y, viewport.width, viewport.height},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    } else {
        SetTextureFilter(screen_texture, (upscale_mode == noctis::UpscaleMode::smooth_bilinear)
                                             ? TEXTURE_FILTER_BILINEAR
                                             : TEXTURE_FILTER_POINT);
        DrawTexturePro(screen_texture,
                       Rectangle{0.0f, 0.0f, static_cast<float>(adapted_width), static_cast<float>(adapted_height)},
                       Rectangle{viewport.x, viewport.y, viewport.width, viewport.height},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }

    noctis::end_crt_shader();

    if (!noctis::render_overlay_notice(render_w, render_h, viewport) && fcs_status_delay > 0) {
        noctis::render_high_dpi_hud(reinterpret_cast<const char *>(fcs_status_extended),
                                    fcs_status_delay, render_w, render_h, viewport);
    }
    noctis::render_timewarp_slider(render_w, render_h, viewport, fcs_status_delay);
    noctis::render_volume_slider_overlay(render_w, render_h, viewport, fcs_status_delay,
                                         graphics_menu_status == 2);
    noctis::render_gallery_viewer(render_w, render_h, viewport);

    // Frame limiter: 18.2 FPS canonical simulation tick (55 ms);
    // 62.5 FPS (~16 ms) during timewarp or on observation deck when ROOFSPEED is enabled.
    const auto goal = std::chrono::milliseconds(
        ((ontheroof != 0 && roof_speed != 0) || noctis::is_timewarp_active()) ? 16 : FRAME_TIME_MILLIS);
    static auto next_frame     = std::chrono::steady_clock::now() + goal;
#ifdef __EMSCRIPTEN__
    // Finish the frame, then hand control back to the browser (ASYNCIFY) so it
    // can present the canvas and deliver input before the next tick.
    EndDrawing();
    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(next_frame - std::chrono::steady_clock::now());
    emscripten_sleep(static_cast<unsigned int>(std::max<std::int64_t>(0, remaining.count())));

    // Autosave about every 30 s of play, and whenever web/pre.js asks because
    // the tab is being hidden or closed; then flush storage to IndexedDB.
    static int autosave_frames = 0;
    const bool save_requested = EM_ASM_INT({
        const requested = Module.nivlrSaveRequested ? 1 : 0;
        Module.nivlrSaveRequested = false;
        return requested;
    });
    if (save_requested || ++autosave_frames >= 30 * 1000 / FRAME_TIME_MILLIS) {
        autosave_frames = 0;
        if (surface_active) {
            // The surface loop reuses the ship coordinates, so the ship state was
            // saved on landing and only the surface position is recorded now.
            surface_autosave_due = true;
        } else {
            freeze();
            remove(surface_file);
            remove(native_surface_file);
        }
        persist_browser_storage();
    }
    const auto now = std::chrono::steady_clock::now();
    next_frame += goal;
    if (next_frame < now)
        next_frame = now + goal;
#else
    std::this_thread::sleep_until(next_frame);
    const auto now = std::chrono::steady_clock::now();
    next_frame += goal;
    if (next_frame < now)
        next_frame = now + goal;
    EndDrawing();
#endif
}

void loop() {
    // Check the flag that indicates when you are on the "observation deck",
    // the roof of the Stardrifter.
    static int16_t prev_lifter = 0;
    if (lifter != 0 && prev_lifter == 0) {
        noctis::play_deck_lift();
    }
    prev_lifter = lifter;
    pos_y += lifter;

    if (lifter > 0) {
        lifter--;

        if (lifter > 65) {
            user_alfa += 0.11 * (40 - user_alfa);
        } else {
            user_alfa -= 0.25 * user_alfa;
        }

        step = 0.5 * lifter;
    }

    if (lifter < 0) {
        lifter++;

        if (pos_y > -325) {
            user_alfa += 0.12 * (user_alfa - 40);
        }

        if (pos_y < -325 && pos_y > -715) {
            step = -pos_y;
        }
    }

    if (pos_y > 0) {
        lifter = 0;
        pos_y  = 0;
    }

    if (pos_y < -750) {
        lifter = 0;
        pos_y  = -750;
    }

    if (pos_y < -500) {
        ontheroof = 1;
    } else {
        ontheroof = 0;
    }

    if (!lifter && ontheroof) {
        DfCoS = pos_z + 3100;
        DfCoS = sqrt(pos_x * pos_x + DfCoS * DfCoS);

        if (DfCoS + step < 1100) {
            lifter = +75;
        }
    }

    /*
     * Response to on-board operating system reset.
     * The "reset_signal" variable controls the reset
     * procedure, which resets all operating parameters
     * to their original state.
     *
     * Setting reset_signal to 150 resets the whole system.
     * Setting reset_signal to 60 resets the GOES network.
     */
    if (reset_signal) {
        switch (reset_signal) {
        case 150:
            elight  = 1;
            ilight  = 0;
            ilightv = 0;
            status("----------", 100);
            break;

        case 140:
            ip_targetted = -1;
            ip_reaching  = 0;
            break;

        case 130:
            ap_targetted = 0;
            stspeed      = 0;
            break;

        case 120:
            gburst             = 0;
            nsync              = 1;
            anti_rad           = 1;
            pl_search          = 0;
            field_amplificator = 0;
            break;

        case 115:
            sys  = 4;
            psys = 4;
            break;

        case 110:
            lithium_collector = 0;
            autoscreenoff     = 0;
            break;

        case 101:
            status("_^*^-!_$[]", 100);
            ap_reached    = 0;
            ip_reached    = 0;
            landing_point = 0;
            break;

        case 75:
            elight  = 0;
            ilightv = 1;
            break;

        case 55:
            mslocate(0, 0, 0);
            mswrite(0, "G.O.E.S. COMMAND NET:");
            break;

        case 35:
            mslocate(0, 0, 1);
            mswrite(0, "REVISION ID 6011/0200");
            break;

        case 25:
            mslocate(0, 0, 2);
            mswrite(0, "SESSION ID ");
            fast_srand(legacy_u32_from_double(secs * 18));
            sprintf(temp_distance_buffer, "%05d%05d", fast_random(0x7FFF), fast_random(0x7FFF));
            mswrite(0, temp_distance_buffer);
            break;

        case 10:
            status("STANDBY", 100);
            break;
        default:
            break;
        }

        reset_signal--;
    }

    //
    // Controlla il timer di sistema.
    //
    getsecs();

    //
    // Accensione luci d'emergenza.
    // Comportamento dell'astrozattera in mancanza di litio.
    //
    if (pwr <= 15000 && !charge) {
        elight             = 1;
        nsync              = 0;
        anti_rad           = 0;
        pl_search          = 0;
        field_amplificator = 0;
        ip_targetted       = -1;
        ap_reached         = 0;
        datasheetdelta     = -100;
    } else {
        if (elight && !reset_signal) {
            elight       = 0;
            ilight       = 0;
            ilightv      = 1;
            reset_signal = 200;
        }
    }

    // Mouse input for user movements.
    p_mpul = mpul;
    handle_input();

    if (noctis::consume_cancel()) {
        if (graphics_menu_status != 0) {
            graphics_menu_status = 0;
            noctis::play_cockpit_button();
        } else if (movie_recorder.menu_open()) {
            movie_recorder.close_menu();
            noctis::play_cockpit_button();
        } else if (about != 0) {
            about = 0;
            noctis::play_cockpit_button();
        } else if (landing_point != 0) {
            landing_point = 0;
            status("CANCELLED", 50);
            noctis::play_goesnet_chime(false);
        } else if (manual_target != 0) {
            manual_target = 0;
            ap_targetted = 0;
            status("CANCELLED", 50);
            noctis::play_goesnet_chime(false);
        } else if (ap_targetting || ip_targetting) {
            ap_targetting = 0;
            ip_targetting = 0;
            status("CANCELLED", 50);
            noctis::play_goesnet_chime(false);
        } else if (labstar) {
            labstar = 0;
            ap_target_previd = -1;
        } else if (labplanet) {
            labplanet = 0;
            prev_planet_id = -1;
        } else if (data) {
            datasheetdelta = -2;
        } else if (active_screen == 0 && gnc_pos > 0) {
            gnc_pos = 0;
            goesnet_command[0] = 0;
            status("CANCELLED", 50);
        }
    }

    // Dev Bryce: Mouse Currently only rotates camera
    /*if (mpul & 2u) {
        //shift += 3 * mdltx;
    //shift = mdlty;
        dlt_alfa -= (float)mdlty / 8;
    } else {
        //step -= 3 * mdlty;

        if (abs(mdlty) > 7) {
            dlt_alfa = -user_alfa / 6;
        }

        dlt_beta -= (float)mdltx / 3;
    }*/

    const auto mouse_control =
        noctis::space_mouse_control(option_mouse_look, (mpul & 2U) != 0, mdltx, mdlty, user_alfa);
    shift += mouse_control.shift;
    step += mouse_control.step;
    dlt_alfa += mouse_control.pitch;
    dlt_beta += mouse_control.yaw;

    // Left-right Movement
    // shift = ;

    // Left-right Camera Rotation
    // dlt_beta;

    // Up-Down Camera Rotation (Does that make sense?)
    // dlt_alfa

    const int WASD_speed = 20;

    if ((active_screen == -1) && ((labstar == 0) && (labplanet == 0))) {
        // +X / -X Direction
        int8_t x_dir = ((int8_t) key_move_dir.right) - ((int8_t) key_move_dir.left);
        // +Z / -Z Direction
        int8_t z_dir = ((int8_t) key_move_dir.forward) - ((int8_t) key_move_dir.backward);

        if (x_dir || z_dir) {
            step += z_dir * WASD_speed;
            shift += x_dir * WASD_speed;
        }
    }

    if (!noctis::is_mouse_locked_and_focused()) {
        dlt_alfa = 0.0f;
        dlt_beta = 0.0f;
        shift    = 0;
        step     = 0;
    }

    // Mouse input for double left and right click.
    if (ontheroof) {
        goto nop;
    }

    if ((mpul & 1u) && !(p_mpul & 1u)) {
        lselect = 1;
    } else {
        lselect = 0;
    }

    if ((mpul & 2u) && !(p_mpul & 2u) && !right_dblclick) {
        const auto click_time = std::chrono::steady_clock::now();
        if (right_dblclick_timing == std::chrono::steady_clock::time_point{}) {
            right_dblclick_timing = click_time;
            rselect               = 1;
        } else {
            if (click_time - right_dblclick_timing < std::chrono::milliseconds(250)) {
                right_dblclick     = true;
                right_dblclick_dir = user_beta;
            } else {
                right_dblclick_timing = click_time;
                rselect               = 1;
            }
        }
    } else {
        rselect = 0;
    }

    // Disable double clicking while a screen is selected to fix a few weird
    // quirks.
    right_dblclick = (right_dblclick && (active_screen == -1));

    if (right_dblclick) {
        if (ap_targetting) {
            ap_targetting  = 0;
            right_dblclick = false;
            extract_ap_target_infos();
            fix_remote_target();
            goto nop;
        }

        if (ip_targetting) {
            ip_targetting  = 0;
            right_dblclick = false;

            if (ip_targetted != -1) {
                fix_local_target();
            } else {
                status("NO TARGET", 50);
            }

            goto nop;
        }

        if (!holdtomiddle) {
            float xx, zz;
            if (right_dblclick_dir > -135 && right_dblclick_dir < -45) {
                user_beta += 90;
                user_beta /= 1.5;
                xx = pos_x - 2900;
                pos_x -= xx * 0.25;

                if (landing_point) {
                    zz = pos_z + 104 * 15 + 1980;
                } else {
                    zz = pos_z + 1940;
                }

                pos_z -= zz * 0.25;

                if (fabs(xx) < 25 && fabs(zz) < 25 && fabs(user_beta) < 1) {
                    right_dblclick_timing = {};
                    right_dblclick        = false;
                }

                user_beta -= 90;
            } else {
                user_beta /= 1.5;
                zz = pos_z + 500;
                pos_z -= zz * 0.25;

                if (sys != 4) {
                    if (fabs(zz) < 25 && fabs(user_beta) < 1) {
                        right_dblclick_timing = {};
                        right_dblclick        = false;
                    }
                } else {
                    xx = pos_x + 1700;
                    pos_x -= xx * 0.25;

                    if (fabs(zz) < 25 && fabs(xx) < 25 && fabs(user_beta) < 1) {
                        right_dblclick_timing = {};
                        right_dblclick        = false;
                    }
                }
            }
        }
    }

//
// Visual angle variation.
//
nop:
    user_alfa += dlt_alfa;
    dlt_alfa /= 1.5;

    if (fabs(dlt_alfa) < 0.25) {
        dlt_alfa = 0;
    }

    if (user_alfa < -44.9) {
        user_alfa = -44.9;
        dlt_alfa  = 0;
    }

    if (user_alfa > 44.9) {
        user_alfa = 44.9;
        dlt_alfa  = 0;
    }

    user_beta += dlt_beta;
    dlt_beta /= 1.5;

    if (fabs(dlt_beta) < 0.25) {
        dlt_beta = 0;
    }

    if (user_beta > 180) {
        user_beta -= 360;
    }

    if (user_beta < -180) {
        user_beta += 360;
    }

    // Variation of the user's position in the spacecraft.
    alfa = user_alfa;
    beta = user_beta - 90;
    change_angle_of_view();
    p_forward(shift);
    beta = user_beta;
    change_angle_of_view();
    p_forward(step);
    shift /= 1.5;

    if (fabs(shift) < 0.5) {
        shift = 0;
    }

    step /= 1.25;

    if (fabs(step) < 0.5) {
        step = 0;
    }

    if (pos_x < -3100) {
        pos_x = -3100;
    }

    if (pos_x > +3100) {
        pos_x = +3100;
    }

    if (pos_z > -300) {
        pos_z = -300;
    }

    if (pos_z < -5800) {
        pos_z = -5800;
    }

    // Black background, which will be made hazy.
    if (!stspeed) {
        memset(adapted + 2880, 0, (adapted_width * adapted_height) - 2880);
    } else {
        pfade(adapted, 180, 8);
    }

    // Close star management
    proj_from_vehicle();
    dxx   = dzat_x - nearstar_x;
    dyy   = dzat_y - nearstar_y;
    dzz   = dzat_z - nearstar_z;
    l_dsd = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;
    satur = (12 * dsd) / nearstar_ray;
    fast_srand(legacy_u32_from_double(nearstar_z));
    ir = fast_random(31) + 29;

    if (satur < ir) {
        satur = ir;
    }

    if (satur > 63) {
        satur = 63;
    }

    if (l_dsd < 100 * nearstar_ray)
        white_globe(adapted, nearstar_x, nearstar_y, nearstar_z, 3 * nearstar_ray, 0.3);

    for (ir = 0; ir < nearstar_nop; ir++) {
        if (nearstar_p_type[ir] == 10) {
            planet_xyz(ir);
            p_dsd = nearstar_p_qsortdist[ir];
            fast_srand(legacy_u32_from_double(ir + nearstar_x));
            white_globe(adapted, plx, ply, plz, 3 * nearstar_p_ray[ir], 0.15 - fast_flandom() * 0.3);

            if (p_dsd > 5 * nearstar_p_ray[ir] && p_dsd < 1000 * nearstar_p_ray[ir])
                lens_flares_for(dzat_x, dzat_y, dzat_z, plx, ply, plz, (10 * nearstar_p_ray[ir]) / p_dsd,
                                (int16_t) (1 + (0.001 * p_dsd)), 1, 0, 3, 0);
        }
    }

    if (l_dsd > 6 * nearstar_ray) {
        if (nearstar_class != 5 && nearstar_class != 6 && nearstar_class != 10) {
            if (nearstar_class != 11 || gl_start < 90) {
                if (l_dsd > 5 * nearstar_ray && l_dsd < 1000 * nearstar_ray) {
                    lens_flares_for(dzat_x, dzat_y, dzat_z, nearstar_x, nearstar_y, nearstar_z,
                                    (10 * nearstar_ray) / l_dsd, (int16_t) (1 + (0.001 * l_dsd)), 1, 0, 3, 0);
                }
            }
        }

        psmooth_grays_ex(adapted + 2880);
    }

    mask_pixels_ex(adapted, 2880, 64);

    if (l_dsd < 8 * nearstar_ray) {
        if (farstar) {
            farstar = 0;
            load_starface();
        }

        glass_bubble = 0;

        if (nearstar_spin) {
            gl_start += nearstar_spin;
            gl_start %= 360;
            globe(gl_start, adapted, s_background, (uint8_t *) n_globes_map, gl_bytes, nearstar_x, nearstar_y,
                  nearstar_z, nearstar_ray, 64, (int8_t) satur);
        } else {
            globe((simulation_ticks() / 360) % 360, adapted, s_background, (uint8_t *) n_globes_map, gl_bytes,
                  nearstar_x, nearstar_y, nearstar_z, nearstar_ray, 64, (int8_t) satur);
        }
    } else {
        farstar = 1;
    }

    if (l_dsd > 100 * nearstar_ray && l_dsd < 1550 * nearstar_ray) {
        ir = (int32_t) (((1600 * nearstar_ray) - l_dsd) / (100 * nearstar_ray));

        if (ir < 0) {
            ir = 0;
        }

        ir += 0x30;
        far_pixel_at(nearstar_x, nearstar_y, nearstar_z, 0, ir);
        far_pixel_at(nearstar_x, nearstar_y, nearstar_z, 0, ir);
        far_pixel_at(nearstar_x, nearstar_y, nearstar_z, 0, ir);
    }

    //
    // Reflections of the protagonist on the glass (removed).
    /*
        if (!stspeed) {
        if (ilight) {
            from_user ();
            if (cam_z>-1000) {
                if (alfa>-10&&alfa<10)
                    user_drawing_range = 55;
                else
                    user_drawing_range = 70;
            }
            else
                user_drawing_range = 40;
            if (beta>-user_drawing_range&&beta<user_drawing_range) {
                cam_x = -4*beta;
                cam_y = 10*alfa;
                cam_z = 0;
                Forward (2*pos_z);
                alfa *= 2; beta *= 2;
                change_angle_of_view ();
                flares = 2;
                user ();
                flares = 0;
            }
        }
        }
    */
    // Controllo gestore (indicando i comandi con lo sguardo).
    //
    proj_from_user();
    leftturn  = 0;
    rightturn = 0;
    infoarea  = 0;
    s_control = 0;
    s_command = 0;

    if (ontheroof) {
        goto jpr;
    }

    float xx, zz;
    do {
        zz = fabs(cam_z);
        xx = fabs(cam_x);
        forward(zz / 2);
    } while (zz > 25 && xx < 3000);

    if (zz < 25) {
        if (cam_x < -44 * 30) {
            if (cam_x > -68 * 30) {
                s_control = (int16_t) ((cam_y + 25) / 50 + 3);

                if (s_control < 1) {
                    s_control = 1;
                }

                if (s_control > 4) {
                    s_control = 4;
                }

                if (lselect) {
                    if (!ap_targetting && !ip_targetting) {
                        aso_countdown = 100;
                        sys           = s_control;
                        dev_page      = 0;
                    }
                }
            }
        } else {
            if (cam_y < -50) {
                if (cam_x < 68 * 30) {
                    s_command = (int16_t) ((cam_x + 44 * 30) / (27 * 30) + 1);

                    if (s_command < 1) {
                        s_command = 1;
                    }

                    if (s_command > 4) {
                        s_command = 4;
                    }

                    if (lselect) {
                        if (!ap_targetting && !ip_targetting) {
                            aso_countdown = 100;
                            commands();
                            goto jpr;
                        }
                    }
                }
            } else {
                infoarea = 1;
            }
        }
    }

    if (lselect && pwr > 15000) {
        if (revcontrols) {
            if (cam_x > 2500) {
                dlt_nav_beta += 1.5;
                status("PITCH - R", 25);
            }

            if (cam_x < -2500) {
                dlt_nav_beta -= 1.5;
                status("PITCH - L", 25);
            }
        } else {
            if (cam_x > 2500) {
                dlt_nav_beta -= 1.5;
                status("PITCH - L", 25);
            }

            if (cam_x < -2500) {
                dlt_nav_beta += 1.5;
                status("PITCH - R", 25);
            }
        }
    }

    if (cam_x > 2500) {
        rightturn = 1;
    }

    if (cam_x < -2500) {
        leftturn = 1;
    }

//
// Rotazione della navicella.
// Attivazione schermi.
//
jpr:

    if (!elight) {
        // Paratia destra:
        if (user_beta > -135 && user_beta < -45 && pos_z < -104 * 15 && pos_z > -262 * 15 && pos_x > 172 * 15) {
            if (rselect) {
                if (active_screen == -1) {
                    active_screen = (int8_t) ((pos_z + 104 * 15) / (-54 * 15));
                    status("SELECTED", 50);
                    noctis::play_cockpit_button();
                } else {
                    active_screen = -1;
                    status("DESELECTED", 50);
                    noctis::play_cockpit_button();
                }
            }
        } else {
            active_screen = -1;
        }

        // Paratia sinistra:
        // if (user_beta > +45 && user_beta < +135 && pos_z < -104*15 &&
        // pos_z > -154*15 && pos_x < -172*15) active_screen = (pos_z +
        // 104*15) / (-54*15) + 2;
    }

    navigation_beta += dlt_nav_beta;
    dlt_nav_beta /= 1.1;

    if (fabs(dlt_nav_beta) < 0.5) {
        dlt_nav_beta = 0;
    }

    if (navigation_beta >= 360) {
        navigation_beta -= 360;
    }

    if (navigation_beta < 0) {
        navigation_beta += 360;
    }

    // Planet tracking
    proj_from_vehicle();
    draw_planets();

    // Controlling help requests.
    if ((helptime != 0) && secs > helptime) {
        if (gburst) {
            status("HELP CAME!", 50);
            gburst = 0;
        }

        if (secs < helptime + 120) {
            stz = 0;

            if (secs < helptime + 20) {
                stz = pow(helptime + 20 - secs, 2) * 2000;
            }

            if (secs > helptime + 100) {
                stz = pow(helptime + 100 - secs, 2) * 2000;
            }

            if ((stz == 0) && charge < 3) {
                charge = 3;
            }

            other_vehicle_at((stz + 16000) * cos(secs / 10), 4000 * sin(secs / 100), (stz + 16000) * sin(secs / 10));
        } else {
            helptime = 0;
        }
    }

    //
    // Tracciamento della navicella.
    //
    proj_from_user();
    vehicle(opencapcount);

    //
    // Tracciamento riflessi, aggiornamento dello schermo
    // del gestore, tracciamento dello schermo del gestore,
    // reazione visiva agli eventi interni alla navicella.
    //
    if (ontheroof) {
        goto ext_1;
    }

    proj_from_user();

    if (!opencapcount) {
        reflexes();
    }

    if (!(simulation_ticks() % 10)) {
        clear_onboard_screen();
        control(0, "flight control drive");
        control(1, "onboard devices");
        control(2, "preferences");
        control(3, "disable display");

        switch (sys) {
        case 1:
            control(0, "FLIGHT CONTROL DRIVE");

            if (sys != psys) {
                status("FCS MENU", 50);
            }

            fcs();
            break;

        case 2:
            control(1, "ONBOARD DEVICES");

            if (sys != psys) {
                status("SELECT SUB", 50);
            }

            devices();
            break;

        case 3:
            control(2, "PREFERENCES");

            if (sys != psys) {
                status("PREFS MENU", 50);
            }

            prefs();
            break;

        case 4:
            control(3, "DISABLE DISPLAY");

            if (sys != psys) {
                status("SCREEN OFF", 50);
            }
            break;
        default:
            break;
        }

        psys = sys;
    }

    if (!ap_targetting && !ip_targetting) {
        setfx(4);
        dxx = pos_z / 88;

        if (dxx < -16) {
            dxx = -16;
        }

        entity = static_cast<uint8_t>(legacy_u32_from_double(dxx));
        screen();
        setfx(0);
    }

    if (leftturn) {
        arrowcolor = 127 - 16 * (simulation_ticks() % 4);
        digit_at('-', -2900, -50, 12, arrowcolor, 0);

        if (revcontrols) {
            digit_at('>', -3000, -50, 12, arrowcolor, 0);
        } else {
            digit_at('<', -3000, -50, 12, arrowcolor, 0);
        }
    }

    if (rightturn) {
        arrowcolor = 127 - 16 * (simulation_ticks() % 4);
        digit_at('-', +2900, -50, 12, arrowcolor, 0);

        if (revcontrols) {
            digit_at('<', +3000, -50, 12, arrowcolor, 0);
        } else {
            digit_at('>', +3000, -50, 12, arrowcolor, 0);
        }
    }

    //
    // ***** H.U.D. OUTER LAYER *****
    // Fornisce informazioni sullo strato esterno dell'H.U.D.
    // Qualsiasi glifo verr� in seguito trattato con dithering.
    //
    if (active_screen != -1 || !draw_hud) {
        goto nohud_1;
    }

    /* Additional information and diagrams on the H.U.D. , label tracking of
     * selected star, and distance tracking from selected star.
     */
    if (ap_targetting || ap_targetted) {
        alfa = 0;
        beta = 0;
        change_angle_of_view();
        cam_x = 450;
        cam_y = 250;
        cam_z = -750;

        for (mc = 0; mc < 24; mc++) {
            if (labstar && mc == labstar_char) {
                digit_at('_', -6, -15, 5, 127 - 2 * (simulation_ticks() % 32), 0);
            }

            digit_at(star_label[mc], -6, -15, 5, 127, 1);
            cam_x -= 40;
        }

        dxx   = dzat_x - ap_target_x;
        dyy   = dzat_y - ap_target_y;
        dzz   = dzat_z - ap_target_z;
        l_dsd = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) * 5E-5;

        if (ap_reached && ap_target_id == nearstar_identity) {
            l_dsd *= 0.01;
        }

        sprintf(temp_distance_buffer, "%01.2f", l_dsd);
        cam_x = 450;
        cam_y = -180;
        cam_z = -750;
        mc    = 0;

        while (temp_distance_buffer[mc] != 0) {
            digit_at(temp_distance_buffer[mc], -6, -15, 5, 127, 1);
            cam_x -= 40;
            mc++;
        }

        cam_x -= 40;
        digit_at('L', -6, -15, 5, 112, 1);
        cam_x -= 40;
        digit_at('.', -6, -15, 5, 112, 1);
        cam_x -= 40;
        digit_at('Y', -6, -15, 5, 112, 1);
        cam_x -= 40;
        digit_at('.', -6, -15, 5, 112, 1);
    }

    //
    // Tracciamento label del pianeta selezionato,
    // tracciamento distanza dal pianeta selezionato,
    // aggiornamento nome del pianeta-bersaglio.
    //
    if (ip_targetted != -1) {
        update_planet_label();
        alfa = 0;
        beta = 0;
        change_angle_of_view();
        cam_x = 450;
        cam_y = 180;
        cam_z = -750;

        for (mc = 0; mc < 24; mc++) {
            if (labplanet && mc == labplanet_char) {
                digit_at('_', -6, -15, 5, 127 - 2 * (simulation_ticks() % 32), 0);
            }

            digit_at(planet_label[mc], -6, -15, 5, 112, 1);
            cam_x -= 40;
        }

        planet_xyz(ip_targetted);
        dxx   = dzat_x - plx;
        dyy   = dzat_y - ply;
        dzz   = dzat_z - plz;
        l_dsd = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) * 1E-2;
        sprintf(temp_distance_buffer, "%01.2f", l_dsd);
        cam_x = 450;
        cam_y = -250;
        cam_z = -750;
        mc    = 0;

        while (temp_distance_buffer[mc] != 0) {
            digit_at(temp_distance_buffer[mc], -6, -15, 5, 120, 1);
            cam_x -= 40;
            mc++;
        }

        cam_x -= 40;
        digit_at('D', -6, -15, 5, 105, 1);
        cam_x -= 40;
        digit_at('Y', -6, -15, 5, 105, 1);
        cam_x -= 40;
        digit_at('A', -6, -15, 5, 105, 1);
        cam_x -= 40;
        digit_at('M', -6, -15, 5, 105, 1);
        cam_x -= 40;
        digit_at('S', -6, -15, 5, 105, 1);
    }

    //
    // Messaggio di reset, lampeggiante.
    //
    if (reset_signal && (reset_signal % 10) < 5) {
        alfa = 0;
        beta = 0;
        change_angle_of_view();
        cam_x = 300;
        cam_y = 0;
        cam_z = -750;
        mc    = 0;

        while (sr_message[mc] != 0) {
            digit_at(sr_message[mc], -6, -15, 8, 127, 1);
            cam_x -= 60;
            mc++;
        }
    }

//
// Tracing the current FCS status.
//
nohud_1:
    alfa = 0;
    beta = 0;
    change_angle_of_view();
    cam_x = -512;
    cam_y = -275;
    cam_z = -750;
    mc    = strlen(reinterpret_cast<char *>(fcs_status_extended)) - 1;

    while (mc >= 0 && (draw_hud || graphics_menu_status || about || movie_recorder.menu_open())) {
        digit_at(fcs_status_extended[mc], -6, -15, 6, 120, 1);
        cam_x += 45;
        mc--;
    }

    //
    // Link alla funzione di ricerca dei targets in real-time.
    //
    if (collecting_targets) {
        status("SCANNING..", 100);
        collect_targets();

        if (!collecting_targets) {
            if (targets) {
                status("DONE!", 100);
            } else {
                status("NO TARGETS", 100);
            }
        }
    }

    //
    // Display / update table "targets in range".
    //
    if (targets_in_range) {
        if (update_targets) {
            tgts_in_show = 0;
            mc           = topmost_target;

            while (targets && mc < targets && tgts_in_show < 3) {
                tgt_label_pos = search_id_code(targets_table_id[mc], 'S');

                if (tgt_label_pos > -1) {
                    FILE *smh = fopen(starmap_file, "rb");

                    if (smh != nullptr) {
                        fseek(smh, tgt_label_pos + 8, SEEK_SET);
                        fread(&target_name[tgts_in_show], 1, 24, smh);
                        fclose(smh);
                        tgts_in_show++;
                    }
                }

                mc++;
            }

            update_targets = 0;
        }

        cam_x = 175;
        cam_y = 40;
        cam_z = -500;
        frame(225, 48, 285, 96, 2, 90);
        cam_y = 8 + 40;
        mc    = 0;

        while (mc < tgts_in_show) {
            cam_x = 35 + 175;

            if (mc == target_line) {
                frame(226 + 35, 0, 277, 30, 1, 120);
            }

            for (ir = 0; ir < 24; ir++) {
                digit_at(target_name[mc][ir], -5, -10, 3.5, 174, 1);
                cam_x -= 23;
            }

            cam_y -= 55;
            mc++;
        }
    }

ext_1: //
    // Anti-aliasing e dithering (error-diffusion).
    // E` un procedimento molto peculiare, che fornisce effetti
    // straordinariamente belli su uno schermo che, di per s�,
    // � poco risolutivo, sia fisicamente che cromaticamente.
    //
    QUADWORDS -= 240;
    psmooth_64_ex(adapted, 200);
    psmooth_64_ex(adapted, 200);
    QUADWORDS += 240;
    //
    // Tracking of all visible stars.
    //
    proj_from_vehicle();
    sky(0x405C);

    //
    // ***** H.U.D. INNER LAYER *****
    // Provides information on the inner layer of the HUD.
    // Any glyph will not be treated with dithering.
    //
    if (datasheetscroll) {
        area_clear(adapted, 11, 85, 0, 0, 1 + datasheetscroll, 9, 72);
        area_clear(adapted, 11, 95, 0, 0, 1 + datasheetscroll, 40, 112);
        mc = (datasheetscroll / 4) - 1;

        if (mc > 0) {
            switch (data) {
            case 1: // remote target data
                if (ap_targetted) {
                    if (ap_targetted == 1) {
                        wrouthud(14, 87, mc, (char *) star_label);
                        tmp_float = 1e-3 * qt_M_PI * ap_target_ray * ap_target_ray * ap_target_ray;
                        tmp_float *= starmass_correction[ap_target_class];

                        if (nearstar_class == 8 || nearstar_class == 9) {
                            fast_srand(legacy_i32_from_double(ap_target_x) % 32000);

                            switch (fast_random(5)) {
                            case 0:
                                tmp_float /= 1 + 5 * fast_flandom();
                                break;

                            case 1:
                                tmp_float /= 1 + fast_flandom();
                                break;

                            case 2:
                                tmp_float *= 1 + fast_flandom();
                                break;

                            case 3:
                                tmp_float *= 1 + 20 * fast_flandom();
                                break;

                            case 4:
                                tmp_float *= 1 + 50 * fast_flandom();
                            }
                        }

                        const auto radius_text = noctis::format_radius(ap_target_ray);
                        wrouthud(14, 97, mc, radius_text.c_str());
                        wrouthud(14, 103, mc, "PRIMARY MASS:");
                        sprintf((char *) outhudbuffer, "%1.8f BAL. M.", tmp_float);
                        wrouthud(14, 109, mc, (char *) outhudbuffer);
                        tmp_float /= 0.38e-4 * ap_target_ray;

                        if (ap_target_class == 6) {
                            tmp_float *= 0.0022;
                        }

                        wrouthud(14, 116, mc, "SURFACE TEMPERATURE:");
                        sprintf((char *) outhudbuffer, "%1.0f@K&%1.0f@C&%1.0f@F", tmp_float + 273.15, tmp_float,
                                tmp_float * 1.8 + 32);
                        wrouthud(14, 122, mc, (char *) outhudbuffer);
                        sprintf((char *) outhudbuffer, "MAJOR BODIES: %d EST.",
                                starnop(ap_target_x, ap_target_y, ap_target_z));
                        wrouthud(14, 129, mc, (char *) outhudbuffer);
                    } else {
                        wrouthud(14, 87, mc, "DIRECT PARSIS TARGET");
                    }
                } else {
                    wrouthud(14, 87, mc, "REMOTE TARGET NOT SET");
                }

                break;

            case 2: // local intarget data
                if (ip_targetted != -1) {
                    wrouthud(14, 87, mc, (char *) planet_label);
                    wrouthud(14, 97, mc, "PERIOD OF ROTATION:");

                    if (nearstar_p_qsortindex[nearstar_nob - 1] == ip_targetted) {
                        if (nearstar_p_rtperiod[ip_targetted] > 0) {
                            p1 = nearstar_p_rtperiod[ip_targetted];
                            p1 *= 360;
                            p2 = p1 / 1000;
                            p2 /= 1000;
                            p3 = p1 / 1000;
                            p3 %= 1000;
                            p4 = p1 % 1000;
                            sprintf((char *) outhudbuffer, "TRIADS %03d:%03d:%03d", p2, p3, p4);
                            wrouthud(14, 103, mc, (char *) outhudbuffer);
                        } else {
                            if (ip_reaching || ip_reached) {
                                if (nearstar_p_type[ip_targetted] != 10) {
                                    wrouthud(14, 103, mc, "COMPUTING...");
                                } else {
                                    wrouthud(14, 103, mc, "NOT RESOLVABLE");
                                }
                            } else {
                                wrouthud(14, 103, mc, "TOO FAR TO ESTIMATE");
                            }
                        }
                    } else {
                        wrouthud(14, 103, mc, "TOO FAR TO ESTIMATE");
                    }

                    wrouthud(14, 113, mc, "PERIOD OF REVOLUTION:");
                    tmp_float = rtp(ip_targetted);
                    p1        = (int32_t) (tmp_float * 1e-9);
                    p2        = (int32_t) (tmp_float * 1e-6);
                    p2 %= 1000;
                    p3 = (int32_t) (tmp_float * 1e-3);
                    p3 %= 1000;
                    p4 = (int32_t) (tmp_float) % 1000;

                    if (p1 < 2) {
                        sprintf((char *) outhudbuffer, "%d EPOCS, %03d:%03d:%03d", p1, p2, p3, p4);
                    } else {
                        if (p1 < 2047) {
                            sprintf((char *) outhudbuffer, "%d EPOCS, %03d:%03d:???", p1, p2, p3);
                        } else {
                            sprintf((char *) outhudbuffer, "%d EPOCS, %03d:???:???", p1, p2);
                        }
                    }

                    wrouthud(14, 119, mc, (char *) outhudbuffer);
                    const auto radius_text = noctis::format_radius(nearstar_p_ray[ip_targetted]);
                    wrouthud(14, 129, mc, radius_text.c_str());
                } else {
                    wrouthud(14, 87, 21, "LOCAL TARGET NOT SET");
                }

                break;

            case 3: // environment data
                wrouthud(14, 87, mc, "EXTERNAL ENVIRONMENT");
                fast_srand(legacy_u32_from_double(secs / 2));
                tmp_float = 16 - dsd * 0.044;
                tmp_float *= fabs(tmp_float);
                tmp_float -= (tmp_float + 273.15) * eclipse;

                if (tmp_float < -269) {
                    tmp_float = fast_flandom() - 269;
                }

                sprintf((char *) outhudbuffer, "TEMP. %1.2f@K", tmp_float + 273.15);
                wrouthud(14, 97, mc, (char *) outhudbuffer);
                sprintf((char *) outhudbuffer, "      %1.2f@C", tmp_float);
                wrouthud(14, 103, mc, (char *) outhudbuffer);
                sprintf((char *) outhudbuffer, "      %1.2f@F", tmp_float * 1.8 + 32);
                wrouthud(14, 109, mc, (char *) outhudbuffer);
                brtl_srand(legacy_u16_from_double(nearstar_identity));

                if (nearstar_class == 6 || nearstar_class == 5) {
                    ir = brtl_random(50);
                    if (nearstar_class == 5) {
                        ir -= (int32_t) (125 / dsd);

                        if (ir <= 0) {
                            ir = 1;
                        }
                    } else {
                        ir -= (int32_t) (25 / dsd);
                    }
                } else {
                    ir = 0;
                }

                sprintf((char *) outhudbuffer, "LI+ IONS: %d MTPD EST.", ir);
                wrouthud(14, 119, mc, (char *) outhudbuffer);

                tmp_float = 50 + (brtl_random(10)) - (brtl_random(10));

                tmp_float *= (1 - eclipse);
                tmp_float *= 100 / dsd;

                if (nearstar_class == 11) {
                    if (gl_start < 90) {
                        tmp_float *= 75 + brtl_random(50);
                    } else {
                        tmp_float *= 50;
                    }
                }

                if (nearstar_class == 10) {
                    tmp_float *= 0.25;
                }

                if (nearstar_class == 9) {
                    tmp_float *= 3;
                }

                if (nearstar_class == 8) {
                    tmp_float *= 1.5;
                }

                if (nearstar_class == 7) {
                    tmp_float *= 25;
                }

                if (nearstar_class == 6) {
                    tmp_float *= 0.01;
                }

                if (nearstar_class == 5) {
                    tmp_float *= 0.1;
                }

                if (nearstar_class == 4) {
                    tmp_float *= 10;
                }

                if (nearstar_class == 3) {
                    tmp_float *= 0.5;
                }

                if (nearstar_class == 2) {
                    tmp_float *= 18;
                }

                if (nearstar_class == 1) {
                    tmp_float *= 5;
                }

                brtl_srand(legacy_u16_from_double(secs));
                tmp_float *= 1 + (float) (brtl_random(100)) * 0.001 - (float) (brtl_random(100)) * 0.001;
                sprintf((char *) outhudbuffer, "RADIATION: %1.1f KR", tmp_float);
                wrouthud(14, 126, mc, (char *) outhudbuffer);
                break;
            default:
                break;
            }
        }
    }

    datasheetscroll += datasheetdelta;

    if (datasheetscroll > 100) {
        datasheetscroll = 100;
    }

    if (datasheetscroll < 0) {
        datasheetscroll = 0;
        data            = 0;
    }

    draw_plus_overlay(false);

    // Draw planetary targeting cross.
    if ((ip_targetted != -1 && !ip_reached) || ip_targetting) {
        planet_xyz(ip_targetted);

        if (far_pixel_at(plx, ply, plz, 0, 1)) {
            const int32_t scale = internal_res_scale;
            uint32_t index = vptr - adapted_width * 2 * scale;

            for (int16_t i = 0; i < 4; i++) {
                int32_t voffset = (i > 1) ? adapted_width : 1;
                int32_t signmod = (i % 2 == 0) ? -1 : 1;

                for (int16_t j = 4 * scale; j < 8 * scale; j++) {
                    adapted[index + signmod * voffset * j] = 126;
                }
            }
        }
    }

    //
    // Star-Target pointing cross, update star-target name, shift interstellar
    // (suplucsi?)
    //
    dxx   = dzat_x - ap_target_x;
    dyy   = dzat_y - ap_target_y;
    dzz   = dzat_z - ap_target_z;
    l_dsd = sqrt(dxx * dxx + dyy * dyy + dzz * dzz);

    if (ap_targetting || l_dsd > 10000) {
        pointer_cross_for(ap_target_x, ap_target_y, ap_target_z);
    }

    if (ap_targetting || ap_targetted) {
        update_star_label();

        if (stspeed == 1) {
            ras = noctis::remote_arrival_radius(ap_targetted == -1, anti_rad, ap_target_ray);
            if (ap_targetted != -1 && l_dsd < 20000 && nsnp) {
                prepare_nearstar();
                nsnp = 0;
            }

            noctis::TravelPosition position{dzat_x, dzat_y, dzat_z};
            noctis::TravelGuidance guidance{ap_target_initial_d, requested_vimana_coefficient,
                                            current_vimana_coefficient, vimana_reaction_time};
            const auto travel =
                noctis::advance_remote_travel(position, {ap_target_x, ap_target_y, ap_target_z}, ras, guidance);
            dzat_x                       = position.x;
            dzat_y                       = position.y;
            dzat_z                       = position.z;
            requested_vimana_coefficient = guidance.requested_coefficient;
            current_vimana_coefficient   = guidance.current_coefficient;
            vimana_reaction_time         = guidance.reaction_time;

            if (travel.arrived) {
                status("CALIBRATED", 50);
                ap_reached            = 1;
                stspeed               = 0;
                g_active_travel_speed = 0.0f;
                g_active_travel_phase = noctis::TravelPhase::arrived;

                std::string sname(reinterpret_cast<const char *>(star_label), 20);
                while (!sname.empty() && sname.back() == ' ') sname.pop_back();
                const double jump_dist_ly = ap_target_initial_d * 5E-5;
                noctis::active_flight_log().record_system_arrival(
                    ap_target_x, ap_target_y, ap_target_z, ap_target_id,
                    sname, ap_target_class, jump_dist_ly);
                noctis::active_flight_log().save_to_file(noctis::runtime_paths().data_dir / "flight_log.json");
            } else {
                status(noctis::travel_phase_status(travel.phase), 0);
                pwr -= travel.power_cost;
                double move_ratio =
                    (guidance.current_coefficient > 0.0) ? (travel.distance / guidance.current_coefficient) : 0.0;
                // Move ratio climbs from ~0.001 at start, up to 100,000 at peak warp, and drops to ~200 during parking
                g_active_travel_speed =
                    std::clamp(static_cast<float>(std::log10(std::max(1.0, move_ratio)) / 5.0), 0.0f, 1.0f);
                g_active_travel_phase = travel.phase;
            }
        }
    }

//
// Sincronizzazione della navicella con i moti planetari,
// spostamenti interplanetari suplucsi.
//
resynctoplanet:

    if (ip_targetted != -1 && pwr > 15000) {
        planet_xyz(ip_targetted);
        dxx = dzat_x - plx;
        dyy = dzat_y - ply;
        dzz = dzat_z - plz;

        if (ip_reached && nsync) {
            status("TRACKING", 0);

            if (nsync == 1) { // fixed-point chase
                ang    = (double) deg * (double) navigation_beta;
                hold_z = 1.8;
            }

            if (nsync == 2) { // far chase
                ang    = (double) deg * (double) navigation_beta;
                hold_z = 5.4;
            }

            if (nsync == 3) { // syncrone orbit
                ang    = (double) nearstar_p_rotation[ip_targetted] * (double) deg;
                hold_z = 1.8 + 0.1 * nearstar_p_ray[ip_targetted];
            }

            if (nsync == 4) { // vimana orbit
                ang    = 7 * secs * (double) deg;
                hold_z = 3.6;
            }

            if (nsync == 5) { // near chase
                ang    = (double) deg * (double) navigation_beta;
                hold_z = 1.2;
            }

            dxx += hold_z * nearstar_p_ray[ip_targetted] * sin(ang);
            dzz -= hold_z * nearstar_p_ray[ip_targetted] * cos(ang);

            if (_delay < 5) {
                dzat_x -= dxx * 0.05;
                dzat_y -= dyy * 0.05;
                dzat_z -= dzz * 0.05;
            } else {
                dzat_x -= dxx;
                dzat_y -= dyy;
                dzat_z -= dzz;
            }
        }

        if (ip_reaching) {
            noctis::TravelPosition position{dzat_x, dzat_y, dzat_z};
            noctis::TravelGuidance guidance{ip_target_initial_d, requested_approach_coefficient,
                                            current_approach_coefficient, reaction_time};
            const auto travel =
                noctis::advance_local_travel(position, {plx, ply, plz}, nearstar_p_ray[ip_targetted], guidance);
            dzat_x                         = position.x;
            dzat_y                         = position.y;
            dzat_z                         = position.z;
            requested_approach_coefficient = guidance.requested_coefficient;
            current_approach_coefficient   = guidance.current_coefficient;
            reaction_time                  = guidance.reaction_time;
            pwr -= travel.power_cost;
            status(noctis::travel_phase_status(travel.phase), 0);

            if (travel.arrived) {
                status("STANDBY", 0);
                ip_reaching           = 0;
                ip_reached            = 1;
                g_active_travel_speed = 0.0f;
                g_active_travel_phase = noctis::TravelPhase::arrived;

                std::string pname(reinterpret_cast<const char *>(planet_label), 20);
                while (!pname.empty() && pname.back() == ' ') pname.pop_back();
                std::string sname(reinterpret_cast<const char *>(star_label), 20);
                while (!sname.empty() && sname.back() == ' ') sname.pop_back();
                const std::int8_t ptype = (ip_targetted >= 0 && ip_targetted < nearstar_nob)
                                              ? nearstar_p_type[ip_targetted] : 0;
                noctis::active_flight_log().record_orbit_arrival(
                    nearstar_x, nearstar_y, nearstar_z, sname,
                    ip_targetted, pname, ptype);
                noctis::active_flight_log().save_to_file(noctis::runtime_paths().data_dir / "flight_log.json");
            } else {
                double move_ratio =
                    (guidance.current_coefficient > 0.0) ? (travel.distance / guidance.current_coefficient) : 0.0;
                // Move ratio peaks at 20.0 during cruise approach, ~0.04 at warmup, and ~0.02 at refining
                g_active_travel_speed = std::clamp(static_cast<float>(move_ratio / 20.0), 0.0f, 1.0f);
                g_active_travel_phase = travel.phase;
            }
        }
    }

    //
    // Gestione del consumo di litio.
    // Consumi supplementari, gestione ricariche.
    //
    additional_consumes();
    //
    // Calcolo della distanza dalla stella pi� vicina,
    // per il controllo su radiazioni, eclissi, temperatura.
    //
    dxx = dzat_x - nearstar_x;
    dyy = dzat_y - nearstar_y;
    dzz = dzat_z - nearstar_z;
    dsd = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 1;

    //
    // Allontanamento d'emergenza della navicella dalla stella.
    //
    if (dsd < (0.44 + (double) (2 * anti_rad)) * nearstar_ray) {
        status("CORRECTION", 100);
        dzat_x += (dxx / dsd) * 0.1;
        dzat_y += (dyy / dsd) * 0.1;
        dzat_z += (dzz / dsd) * 0.1;
    }

    //
    // Manovre di approvvigionamento litio dallo spazio
    // attorno alle stelle di classe 5 o a certe di classe 6.
    // Le stelle di classe 6 sono pi� difficili da sfruttare,
    // ma danno i migliori risultati. Quelle di classe 5 sono
    // sempre adatte, ma con scarsi risultati.
    //
    if (lithium_collector) {
        brtl_srand(legacy_u16_from_double(nearstar_identity));
        ir = brtl_random(50);

        if (nearstar_class == 5) {
            ir -= (int32_t) (125 / dsd);

            if (ir <= 0) {
                ir = 1;
            }
        } else {
            ir -= (int32_t) (25 / dsd);
        }

        if (ir > 0) {
            if (pwr >= 20000 && charge >= 120) {
                status("! FULL !", 100);
                lithium_collector = 0;
            } else {
                pwr += ir;

                if (pwr >= 20000) {
                    if (charge < 120) {
                        pwr = 15001;
                        charge++;
                    } else {
                        pwr = 20000;
                    }
                }

                status("SCOPING...", 25);
            }
        } else {
            status("GET CLOSER", 100);

            if (!reset_signal) {
                lithium_collector = 0;
            }
        }
    }

    // Eclipse control.
    stz = dzz * cos(deg * navigation_beta) - dxx * sin(deg * navigation_beta) - fabs(dyy) / 2;

    if (stz > dsd) {
        stz = dsd;
    }

    if (stz < 0) {
        stz = 0;
    }

    stz /= 1.25;
    ras = (105 * nearstar_ray) / dsd;

    if (ras > 66) {
        ras = 66;
    }

    if (ras < 1) {
        ras = 1;
    }

    eclipse = 0;

    for (mc = 0; mc < nearstar_nob; mc++) {
        if (nearstar_p_type[mc] != -1) {
            planet_xyz(mc);
            dxx = dzat_x - plx;
            dyy = dzat_y - ply;
            dzz = dzat_z - plz;
            dpz = sqrt(dxx * dxx + dyy * dyy + dzz * dzz) + 0.001;

            if (dpz < 10 * nearstar_p_ray[mc]) {
                watch(dzat_x, dzat_y, dzat_z, nearstar_x, nearstar_y, nearstar_z);
                change_angle_of_view();

                if (xy(dzat_x, dzat_y, dzat_z, plx, ply, plz)) {
                    dasp = sqrt(delta_x * delta_x + delta_y * delta_y);
                    rap  = (105 * nearstar_p_ray[mc]) / dpz;

                    if (rap > 66) {
                        rap = 66;
                    }

                    eclipse = (dasp + ras - rap) / (2 * ras);

                    if (eclipse > 1) {
                        eclipse = 1;
                    }

                    if (eclipse < 0) {
                        eclipse = 0;
                    }

                    eclipse = 1 - eclipse;
                }
            }
        }
    }

    //
    // Reactions to eclipses (obscure the color of the spacecraft).
    // Also lower the internal temperature by a little, but not
    // too much because it is contrasted by air conditioning.
    //
    fast_srand(legacy_u32_from_double(secs / 2));
    pp_temp = 90 - dsd * 0.33;
    pp_temp -= 44;
    pp_temp *= fabs(pp_temp * 0.44);
    pp_temp -= (pp_temp + 273.15) * eclipse;

    if (pp_temp < -269) {
        pp_temp = fast_flandom() - 269;
    }

    if (!ontheroof) {
        if (pp_temp < 0) {
            pp_temp = 0;
        }

        if (pp_temp > 40) {
            pp_temp = 40;
        }

        while (pp_temp < 14) {
            pp_temp += fast_flandom() * 2.5;
        }

        while (pp_temp > 32) {
            pp_temp -= fast_flandom() * 2.5;
        }

        l_dsd = (24 * nearstar_ray) / (dsd - stz);
        l_dsd -= l_dsd * eclipse;
        pp_temp += l_dsd;

        if (pp_temp > 40) {
            pp_temp = 40;
        }

        while (pp_temp > 38) {
            pp_temp -= fast_flandom() * 2.5;
        }

        pp_pressure = 1;
    } else {
        pp_pressure = 0;
    }

    //
    ilight += ilightv;

    if (ilight < 0) {
        ilight = 0;
    }

    if (ilight > 63) {
        ilight = 63;
    }

    l_dsd = (15 * nearstar_ray * nearstar_r) / (dsd - stz);
    l_dsd -= l_dsd * eclipse;

    if (elight) {
        ir3 = (int32_t) (ilight + 30 - simulation_ticks() % 30 + l_dsd);
    } else {
        ir3 = (int32_t) (ilight / 4.0 + l_dsd);
    }

    if (ir3 > nearstar_r + 16) {
        ir3 = nearstar_r + 16;
    }

    if (nearstar_class == 11 && gl_start < 90) {
        ir = 5 * ir3;
    }

    if (ir3 > 63) {
        ir3 = 63;
    }

    l_dsd = (7 * nearstar_ray * nearstar_g) / (dsd - stz);
    l_dsd -= l_dsd * eclipse;

    if (elight) {
        ig3 = (int32_t) ((ilight + 30 - simulation_ticks() % 30) / 2.0 + l_dsd);
    } else {
        ig3 = (int32_t) (ilight / 2.0 + l_dsd);
    }

    if (ig3 > nearstar_g + 32) {
        ig3 = nearstar_g + 32;
    }

    if (nearstar_class == 11 && gl_start < 90) {
        ig3 = 5 * ig3;
    }

    if (ig3 > 63) {
        ig3 = 63;
    }

    l_dsd = (7 * nearstar_ray * nearstar_b) / (dsd - stz);
    l_dsd -= l_dsd * eclipse;

    if (elight) {
        ib3 = (int32_t) ((ilight + 30 - simulation_ticks() % 30) / 4.0 + l_dsd);
    } else {
        ib3 = (int32_t) (ilight + l_dsd);
    }

    if (nearstar_class == 11 && gl_start < 90) {
        ib3 = 5 * ib3;
    }

    if (ib3 > 63) {
        ib3 = 63;
    }

    if (gburst > 0 && gburst < 5) {
        ir3 += 8 * gburst;
        ig3 += 8 * gburst;
        ib3 += 8 * gburst;
    }

    if (ir3 != ir3e || ig3 != ig3e || ib3 != ib3e) {
        tavola_colori(range8088, 0, 64, ir3, ig3, ib3);
        ir3e = ir3;
        ig3e = ig3;
        ib3e = ib3;
    }

    //
    // Controllo del flag di richiesta di atterraggio.
    //
    if (land_now) {
        land_now       = 0;
        landing_point  = 0;
        holdtomiddle   = 1;
        opencapdelta   = 2;
        right_dblclick = false;
        status("UNLOCKING", 50);
    }

    //
    // Avanzamento contatore per le richieste d'aiuto.
    //
    if (gburst > 0) {
        gburst--;

        if (!gburst) {
            gburst = 63;
            status("SIGNAL", 50);
        }
    }

    //
    // Countdown per il delay dei messaggi di stato dell'FCS.
    //
    if (fcs_status_delay) {
        fcs_status_delay--;
    }

    //
    // Countdown per il delay della funzione auto-sleep.
    //
    if (autoscreenoff) {
        aso_countdown--;

        if (aso_countdown <= 0) {
            aso_countdown = 100;
            sys           = 4;
        }
    }

    //
    // Il protagonista sta sempre in una tutina... � normale.
    // Vede le cose attraverso uno scafandro, non ingombrante
    // ma pur sempre uno scafandro. La funzione "surrounding"
    // disegna i bordi dello scafandro, illuminati in relazione
    // all'ambiente circostante.
    //
    surrounding(0, adapted_height - 20);
    // riduzione stanchezza (continua, eventualmente
    // dall'ultima volta che si � scesi in superficie)
    // e variazioni nelle pulsazioni, pi� verosimili...
    fast_srand(legacy_u32_from_double(secs / 2));
    tiredness *= 0.9977;
    pp_pulse = (1 + tiredness) * 118;
    pp_pulse += fast_flandom() * 8;
    pp_pulse -= fast_flandom() * 8;

    // se si sta per scendere o si � appena risaliti,
    // si deve trattenere il player nel mezzo della navicella,
    // in quanto si suppone che sia bloccato nella capsula.
    if (holdtomiddle || lifter) {
        pos_x *= 0.75;
        hold_z = pos_z + 3100;
        pos_z -= hold_z * 0.25;
    }

    // effetto di apertura della capsula:
    // quando � totalmente aperta, si pu� scendere.
    // LQ significa Last Quadrant (ultimo quadrante visitato)
    if (opencapdelta < 0) {
        opencapcount += opencapdelta;

        if (opencapcount <= 0) {
            opencapdelta = 0;
            holdtomiddle = 0;
            sprintf(temp_distance_buffer, "LQ %03d:%03d", landing_pt_lon, landing_pt_lat);
            status(temp_distance_buffer, 100);
        }
    }

    // effetto di chiusura della capsula:
    // quando � totalmente sigillata, scotty beam me down.
    // al ritorno, comincia a riaprire la capsula...
    if (opencapdelta > 0) {
        opencapcount += opencapdelta;

        if (opencapcount >= 85) {
            entryflag = 0;
#ifdef __EMSCRIPTEN__
            freeze(); // Ship state for resuming if the tab closes on the surface.
#endif
            planetary_main();

            if (exitflag) {
                freeze();
                exit(0);
            }

            opencapdelta = -2;
            holdtomiddle = 1;
            pp_gravity   = 1;
            resolve      = 0;
            _delay       = 13; // solo al ritorno da superficie
            goto resynctoplanet;
        }
    }

    // Page swap.
    QUADWORDS = (adapted_width * adapted_height) / 4;

    if (_delay == 13) {
        _delay = 0;
    }

    if (!_delay) {
        noctis::AudioTelemetry telemetry{};
        telemetry.scene              = ontheroof ? noctis::AudioScene::roof : noctis::AudioScene::cabin;
        telemetry.travel_active      = (stspeed == 1) || (ip_reaching == 1);
        telemetry.travel_phase       = static_cast<int>(g_active_travel_phase);
        telemetry.travel_speed       = telemetry.travel_active ? g_active_travel_speed : 0.0f;
        telemetry.atmosphere_density = 0.0f;
        telemetry.weather_rain       = 0.0f;
        telemetry.player_walking     = false;
        telemetry.jetpack_active     = false;
        const bool attitude_maneuver = (std::abs(dlt_nav_beta) > 0.05f) ||
                                       (dsd < (0.44 + (double) (2 * anti_rad)) * nearstar_ray);
        telemetry.rcs_active         = !telemetry.travel_active && attitude_maneuver;
        telemetry.entry_buffeting    = 0.0f;
        noctis::update_audio_telemetry(telemetry);

        advance_movie_capture(false);
        swapBuffers();
    } else if (_delay > 0 && _delay < 10) {
        _delay--;
    }

    QUADWORDS = pqw;

    if (resolve == 1) {
        while (resolve <= 63) {
            tavola_colori((uint8_t *) return_palette, 0, 256, resolve, resolve, resolve);
            resolve += 4;
        }
    }

    if (resolve == 0) {
        resolve = 1;
    }

    //
    // This section controls the pixels in a continuous loop
    // depicting the convective currents inside the stars,
    // but goes into action only if the star is close enough,
    // otherwise you can not see a white globe: The colors
    // of the stars are not really colors, but only soft
    // nuances; in practice, being almost all well beyond the
    // saturation level of the eye, the stars appear generally
    // all white, unless you approach them really very much.
    //
    if (dsd < 1000 * nearstar_ray) {
        ir = nearstar_r;
        ig = nearstar_g;
        ib = nearstar_b;
        mc = nearstar_class;

        if (mc == 8) {
            fast_srand(legacy_u32_from_double(nearstar_identity));
            brtl_srand(fast_random(0x7FFF));
            mc = brtl_random(star_classes);
            ir = class_rgb[mc * 3 + 0];
            ig = class_rgb[mc * 3 + 1];
            ib = class_rgb[mc * 3 + 2];
        }

        switch (mc) {
        case 0:
            ir2 = 64;
            ig2 = 54;
            ib2 = 28;
            break;

        case 1:
            ir2 = 36;
            ig2 = 50;
            ib2 = 64;
            break;

        case 2:
            ir2 = 24;
            ig2 = 32;
            ib2 = 48;
            break;

        case 3:
            ir2 = 64;
            ig2 = 24;
            ib2 = 12;
            break;

        case 4:
            ir2 = 64;
            ig2 = 40;
            ib2 = 32;
            break;

        case 5:
            ir2 = 28;
            ig2 = 20;
            ib2 = 12;
            break;

        case 6:
            ir2 = 32;
            ig2 = 32;
            ib2 = 32;
            break;

        case 7:
            ir2 = 32;
            ig2 = 44;
            ib2 = 64;
            break;

        case 8:
            ir2 = 64;
            ig2 = 60;
            ib2 = 32;
            break;

        case 9:
            fast_srand(legacy_u32_from_double(nearstar_identity));
            ir2 = 32 + fast_random(31);
            ig2 = 32 + fast_random(31);
            ib2 = 16 + fast_random(31);
            break;

        case 10:
            ir2 = 32;
            ig2 = 26;
            ib2 = 22;
            break;

        case 11:
            ir2 = 36;
            ig2 = 48;
            ib2 = 64;
            break;
        default:
            break;
        }

        satur = (6.4 * dsd) / nearstar_ray;

        if (satur > 44) {
            satur = 44;
        }

        if (ir < satur) {
            ir = (int32_t) satur;
        }

        if (ig < satur) {
            ig = (int32_t) satur;
        }

        if (ib < satur) {
            ib = (int32_t) satur;
        }

        if (ir2 < satur) {
            ir2 = (int32_t) satur;
        }

        if (ig2 < satur) {
            ig2 = (int32_t) satur;
        }

        if (ib2 < satur) {
            ib2 = (int32_t) satur;
        }
    } else {
        ir  = 48;
        ig  = 56;
        ib  = 64;
        ir2 = 24;
        ig2 = 32;
        ib2 = 40;
    }

    if (ire == ir && ige == ig && ibe == ib && ir2e == ir2 && ig2e == ig2 && ib2e == ib2) {
        if (!sky_palette_ok) {
            sky_palette_ok = 1;
            goto last_sky_palette_redefinition;
        }
    } else {
        sky_palette_ok = 0;
    last_sky_palette_redefinition:

        if (ire < ir) {
            ire++;
        }

        if (ire > ir) {
            ire--;
        }

        if (ige < ig) {
            ige++;
        }

        if (ige > ig) {
            ige--;
        }

        if (ibe < ib) {
            ibe++;
        }

        if (ibe > ib) {
            ibe--;
        }

        if (ir2e < ir2) {
            ir2e++;
        }

        if (ir2e > ir2) {
            ir2e--;
        }

        if (ig2e < ig2) {
            ig2e++;
        }

        if (ig2e > ig2) {
            ig2e--;
        }

        if (ib2e < ib2) {
            ib2e++;
        }

        if (ib2e > ib2) {
            ib2e--;
        }

        shade(tmppal, 64 + 00, 24, 0, 0, 0, ir2e, ig2e, ib2e);
        shade(tmppal, 64 + 24, 16, ir2e, ig2e, ib2e, ire, ige, ibe);
        shade(tmppal, 64 + 40, 24, ire, ige, ibe, 64, 70, 76);
        tavola_colori(tmppal + 3 * 64, 64, 64, 63, 63, 63);
    }

    if (!farstar) {
        for (ir = 0; ir < 64800; ir++) {
            ig               = (s_background[ir] + 1) % 64;
            ib               = ((uint8_t) (s_background[ir] >> 6u)) << 6u;
            s_background[ir] = ig + ib;
        }
    }

    //

    /* Hook for managing the motion characteristics of planet surface features */
    static double last_map_refresh_secs = 0.0;
    static int16_t last_target_rotation = 0;
    if (ip_targetted != -1 && ip_reached) {
        const int16_t cur_rot = nearstar_p_rotation[ip_targetted];
        if (std::abs(secs - last_map_refresh_secs) >= 300.0 ||
            std::abs(cur_rot - last_target_rotation) >= 3) {
            last_map_refresh_secs = secs;
            last_target_rotation = cur_rot;
            npcs = -12345;
        }
    }

    /* Keyboard input: Taking snapshots, ending the game session, selecting
     * targets, attributing labels, etc.
     */
    if (ontheroof) {
        if (is_key()) {
            mc = get_key();
            if (!mc && is_key()) {
                mc = get_key();
                if (mc == 0x3B) {
                    about                = !about;
                    graphics_menu_status = 0;
                    movie_recorder.close_menu();
                } else if (mc == 0x3C) {
                    graphics_menu_status = (graphics_menu_status + 1) % 4;
                    about                = 0;
                    movie_recorder.close_menu();
                } else if (mc == 0x3D || mc == 142 || mc == 144) {
                    handle_movie_extended_key(mc);
                } else if (mc == 0x3E) {
                    open_cockpit_gallery({});
                } else if (graphics_menu_status == 2) {
                    if (mc == 72) {
                        noctis::select_previous_audio_category();
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()));
                        status(msg, 50);
                    } else if (mc == 80) {
                        noctis::select_next_audio_category();
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()));
                        status(msg, 50);
                    } else if (mc == 75) {
                        float v = noctis::step_selected_audio_category_volume(-0.05f);
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s %d%%",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()),
                                      static_cast<int>(std::round(v * 100.0f)));
                        status(msg, 50);
                    } else if (mc == 77) {
                        float v = noctis::step_selected_audio_category_volume(+0.05f);
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s %d%%",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()),
                                      static_cast<int>(std::round(v * 100.0f)));
                        status(msg, 50);
                    }
                } else if (graphics_menu_status == 3) {
                    if (mc == 72 || mc == 80) {
                        const bool inv = noctis::toggle_mouse_inversion();
                        status(inv ? "MOUSE PITCH: INVERTED" : "MOUSE PITCH: NORMAL", 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                    } else if (mc == 75) {
                        const float s = noctis::adjust_mouse_sensitivity(-0.1f);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                        status(msg, 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                    } else if (mc == 77) {
                        const float s = noctis::adjust_mouse_sensitivity(+0.1f);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                        status(msg, 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                    }
                } else if (mc == 75) {
                    dlt_nav_beta += 1.5;
                    status("PITCH - R", 25);
                } else if (mc == 77) {
                    dlt_nav_beta -= 1.5;
                    status("PITCH - L", 25);
                } else if (mc == 72) {
                    lifter = -100;
                } else if (mc == 80) {
                    option_mouse_look = noctis::cycle_mouse_look(option_mouse_look);
                    status(option_mouse_look == 0   ? "MOUSELOOK OFF"
                           : option_mouse_look == 1 ? "MOUSELOOK ON"
                                                    : "INV. Y AXIS",
                           50);
                    noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                }
            } else if (handle_movie_key(mc, false)) {
            } else if (graphics_menu_status == 1) {
                if (mc == 9 || mc == 'a' || mc == 'A') {
                    graphics_menu_status = 2;
                    noctis::touch_volume_slider();
                    noctis::play_cockpit_button();
                } else if (mc == 27) {
                    graphics_menu_status = 0;
                } else if (mc == 't') {
                    draw_hud = !draw_hud;
                    status(draw_hud ? "TEXT ON" : "TEXT OFF", 100);
                    save_display_settings_current();
                } else if (mc == 'f') {
                    lens_flare_mode = noctis::cycle_lens_flare_mode(lens_flare_mode);
                    status(lens_flare_mode == 1    ? "FLARES ON"
                           : lens_flare_mode == -1 ? "FLARES OFF"
                                                   : "VISOR FLARES",
                           100);
                    save_display_settings_current();
                } else if (mc == 'b' || mc == noctis::delete_snapshot_key) {
                    seamless_border = !seamless_border;
                    status(seamless_border ? "SEAMLESS BD." : "DEFAULT BD.", 100);
                    save_display_settings_current();
                } else if (mc == 'u') {
                    const auto new_mode = noctis::cycle_upscale_mode(noctis::get_upscale_mode());
                    noctis::set_upscale_mode(new_mode);
                    status(noctis::upscale_mode_name(new_mode), 100);
                    save_display_settings_current();
                } else if (mc == 'c') {
                    const bool active = noctis::toggle_crt_shader();
                    status(active ? "CRT SHADER: ACTIVE" : "CRT SHADER: DISABLED", 100);
                    save_display_settings_current();
                } else if (mc == 'g') {
                    const bool active = noctis::toggle_subpixel_fidelity();
                    status(active ? "FIDELITY: SUB-PIXEL" : "FIDELITY: LEGACY", 100);
                    save_display_settings_current();
                } else if (mc == 'r' || mc == 'R') {
                    const auto new_mode = noctis::cycle_internal_resolution_mode();
                    status(noctis::internal_resolution_mode_name(new_mode), 100);
                    save_display_settings_current();
                }
            } else if (graphics_menu_status == 2) {
                if (mc == 9) {
                    graphics_menu_status = 3;
                    noctis::play_cockpit_button();
                } else if (mc == 'v' || mc == 'V') {
                    graphics_menu_status = 1;
                    noctis::play_cockpit_button();
                } else if (mc == 27) {
                    graphics_menu_status = 0;
                } else if (mc >= '1' && mc <= '5') {
                    noctis::set_selected_audio_category(static_cast<noctis::AudioCategory>(mc - '1'));
                    noctis::touch_volume_slider();
                    noctis::play_cockpit_button();
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), "%s",
                                  noctis::audio_category_name(noctis::get_selected_audio_category()));
                    status(msg, 50);
                } else if (mc == 'm' || mc == 'M') {
                    noctis::toggle_audio_mute();
                    noctis::touch_volume_slider();
                    noctis::play_cockpit_button();
                    noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                    status(noctis::is_audio_muted() ? "AUDIO MUTED" : "AUDIO UNMUTED", 50);
                } else if (mc == '+' || mc == '=' || mc == ']' || mc == '.' || mc == '>') {
                    float v = noctis::step_selected_audio_category_volume(+0.05f);
                    noctis::touch_volume_slider();
                    noctis::play_cockpit_button();
                    noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), "%s %d%%",
                                  noctis::audio_category_name(noctis::get_selected_audio_category()),
                                  static_cast<int>(std::round(v * 100.0f)));
                    status(msg, 50);
                } else if (mc == '-' || mc == '_' || mc == '[' || mc == ',' || mc == '<') {
                    float v = noctis::step_selected_audio_category_volume(-0.05f);
                    noctis::touch_volume_slider();
                    noctis::play_cockpit_button();
                    noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), "%s %d%%",
                                  noctis::audio_category_name(noctis::get_selected_audio_category()),
                                  static_cast<int>(std::round(v * 100.0f)));
                    status(msg, 50);
                }
            } else if (graphics_menu_status == 3) {
                if (mc == 9 || mc == 'v' || mc == 'V') {
                    graphics_menu_status = 1;
                    noctis::play_cockpit_button();
                } else if (mc == 'a' || mc == 'A') {
                    graphics_menu_status = 2;
                    noctis::play_cockpit_button();
                } else if (mc == 27) {
                    graphics_menu_status = 0;
                } else if (mc == 'i' || mc == 'I') {
                    const bool inv = noctis::toggle_mouse_inversion();
                    status(inv ? "MOUSE PITCH: INVERTED" : "MOUSE PITCH: NORMAL", 50);
                    noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                    noctis::play_cockpit_button();
                } else if (mc == '-' || mc == '_') {
                    const float s = noctis::adjust_mouse_sensitivity(-0.1f);
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                    status(msg, 50);
                    noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                    noctis::play_cockpit_button();
                } else if (mc == '+' || mc == '=') {
                    const float s = noctis::adjust_mouse_sensitivity(+0.1f);
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                    status(msg, 50);
                    noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                    noctis::play_cockpit_button();
                } else if (mc == 'm' || mc == 'M') {
                    option_mouse_look = noctis::cycle_mouse_look(option_mouse_look);
                    status(option_mouse_look == 0 ? "MOUSELOOK OFF" : option_mouse_look == 1 ? "MOUSELOOK ON" : "INV. Y AXIS", 50);
                    noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                    noctis::play_cockpit_button();
                } else if (mc == 'r' || mc == 'R') {
                    const bool r = noctis::toggle_rumble();
                    status(r ? "RUMBLE HAPTICS: ON" : "RUMBLE HAPTICS: OFF", 50);
                    if (r) noctis::trigger_gamepad_rumble(noctis::GamepadRumbleType::click, 1.0f);
                    noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                    noctis::play_cockpit_button();
                }
            } else if (noctis::is_roof_speed_key(mc)) {
                noctis::toggle_timewarp();
                roof_speed = noctis::is_timewarp_active() ? 1 : 0;
                noctis::touch_timewarp_slider();
                char msg[32];
                std::snprintf(msg, sizeof(msg), noctis::is_timewarp_active() ? "TIME %dx" : "REALTIME 1x",
                              noctis::get_timewarp_multiplier());
                status(msg, 50);
            } else if (mc == '[' || mc == ']' || mc == '-' || mc == '+' || mc == '=') {
                const auto m = noctis::step_timewarp_multiplier((mc == '[' || mc == '-') ? -1 : 1);
                noctis::touch_timewarp_slider();
                save_display_settings_current();
                char msg[32];
                std::snprintf(msg, sizeof(msg), "SPEED %dx", m);
                status(msg, 50);
            } else if (noctis::snapshot_action(mc, false, false, false) == noctis::SnapshotAction::normal) {
                snapshot(0, 1);
            } else if (noctis::snapshot_action(mc, false, false, false) == noctis::SnapshotAction::raw) {
                snapshot(0, 0);
            }
        } else {
            mc = 0;
        }

        goto endmain;
    }

    if (goesk_e != -1) {
        mc      = goesk_e;
        goesk_e = -1;
        goto goesk_e_reentry;
    }

    if (goesk_a != -1) {
        mc      = goesk_a;
        goesk_a = -1;
        goto goesk_a_reentry;
    }

    if (active_screen == -1 && is_key()) {
        while (is_key()) {
            mc = get_key();

            if (!mc) {
                mc = get_key();
            goesk_e_reentry:

                if (mc == 0x3B) {
                    about                = !about;
                    graphics_menu_status = 0;
                    movie_recorder.close_menu();
                    goto endmain;
                }

                if (mc == 0x3C) {
                    graphics_menu_status = (graphics_menu_status + 1) % 4;
                    about                = 0;
                    movie_recorder.close_menu();
                    goto endmain;
                }

                if (mc == 0x3D || mc == 142 || mc == 144) {
                    handle_movie_extended_key(mc);
                    goto endmain;
                }

                if (mc == 0x3E) {
                    open_cockpit_gallery({});
                    goto endmain;
                }

                if (graphics_menu_status == 2) {
                    if (mc == 72) {
                        noctis::select_previous_audio_category();
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()));
                        status(msg, 50);
                        goto endmain;
                    }
                    if (mc == 80) {
                        noctis::select_next_audio_category();
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()));
                        status(msg, 50);
                        goto endmain;
                    }
                    if (mc == 75) {
                        float v = noctis::step_selected_audio_category_volume(-0.05f);
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s %d%%",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()),
                                      static_cast<int>(std::round(v * 100.0f)));
                        status(msg, 50);
                        goto endmain;
                    }
                    if (mc == 77) {
                        float v = noctis::step_selected_audio_category_volume(+0.05f);
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s %d%%",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()),
                                      static_cast<int>(std::round(v * 100.0f)));
                        status(msg, 50);
                        goto endmain;
                    }
                } else if (graphics_menu_status == 3) {
                    if (mc == 72 || mc == 80) {
                        const bool inv = noctis::toggle_mouse_inversion();
                        status(inv ? "MOUSE PITCH: INVERTED" : "MOUSE PITCH: NORMAL", 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 75) {
                        const float s = noctis::adjust_mouse_sensitivity(-0.1f);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                        status(msg, 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 77) {
                        const float s = noctis::adjust_mouse_sensitivity(+0.1f);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                        status(msg, 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                }

                if (targets_in_range) {
                    if (mc == 80) {
                        if (target_line < 2) {
                            if (topmost_target + target_line < targets - 1) {
                                target_line++;
                            }
                        } else {
                            if (topmost_target < targets - 3) {
                                topmost_target++;
                                update_targets = 1;
                            }
                        }
                    }

                    if (mc == 72) {
                        if (target_line > 0) {
                            target_line--;
                        } else {
                            if (topmost_target > 0) {
                                topmost_target--;
                                update_targets = 1;
                            }
                        }
                    }

                    goto endmain;
                }

                if (mc == 75) {
                    dlt_nav_beta += 1.5;
                    status("PITCH - R", 25);
                }

                if (mc == 77) {
                    dlt_nav_beta -= 1.5;
                    status("PITCH - L", 25);
                }

                if (mc == 72) {
                    lifter = -100;
                }

                if (mc == 80) {
                    option_mouse_look = noctis::cycle_mouse_look(option_mouse_look);
                    status(option_mouse_look == 0   ? "MOUSELOOK OFF"
                           : option_mouse_look == 1 ? "MOUSELOOK ON"
                                                    : "INV. Y AXIS",
                           50);
                }
            } else {
            goesk_a_reentry:

                if (handle_movie_key(mc, labstar || labplanet))
                    goto endmain;

                if (graphics_menu_status == 1) {
                    if (mc == 9 || mc == 'a' || mc == 'A') {
                        graphics_menu_status = 2;
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 27) {
                        graphics_menu_status = 0;
                        goto endmain;
                    }
                    if (mc == 't') {
                        draw_hud = !draw_hud;
                        status(draw_hud ? "TEXT ON" : "TEXT OFF", 100);
                        save_display_settings_current();
                        goto endmain;
                    }
                    if (mc == 'f') {
                        lens_flare_mode = noctis::cycle_lens_flare_mode(lens_flare_mode);
                        status(lens_flare_mode == 1    ? "FLARES ON"
                               : lens_flare_mode == -1 ? "FLARES OFF"
                                                       : "VISOR FLARES",
                               100);
                        save_display_settings_current();
                        goto endmain;
                    }
                    if (mc == 'b' || mc == noctis::delete_snapshot_key) {
                        seamless_border = !seamless_border;
                        status(seamless_border ? "SEAMLESS BD." : "DEFAULT BD.", 100);
                        save_display_settings_current();
                        goto endmain;
                    }
                    if (mc == 'u') {
                        const auto new_mode = noctis::cycle_upscale_mode(noctis::get_upscale_mode());
                        noctis::set_upscale_mode(new_mode);
                        status(noctis::upscale_mode_name(new_mode), 100);
                        save_display_settings_current();
                        goto endmain;
                    }
                    if (mc == 'c') {
                        const bool active = noctis::toggle_crt_shader();
                        status(active ? "CRT SHADER: ACTIVE" : "CRT SHADER: DISABLED", 100);
                        save_display_settings_current();
                        goto endmain;
                    }
                    if (mc == 'g') {
                        const bool active = noctis::toggle_subpixel_fidelity();
                        status(active ? "FIDELITY: SUB-PIXEL" : "FIDELITY: LEGACY", 100);
                        save_display_settings_current();
                        goto endmain;
                    }
                    if (mc == 'r' || mc == 'R') {
                        const auto new_mode = noctis::cycle_internal_resolution_mode();
                        status(noctis::internal_resolution_mode_name(new_mode), 100);
                        save_display_settings_current();
                        goto endmain;
                    }
                } else if (graphics_menu_status == 2) {
                    if (mc == 9) {
                        graphics_menu_status = 3;
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 'v' || mc == 'V') {
                        graphics_menu_status = 1;
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 27) {
                        graphics_menu_status = 0;
                        goto endmain;
                    }
                    if (mc >= '1' && mc <= '5') {
                        noctis::set_selected_audio_category(static_cast<noctis::AudioCategory>(mc - '1'));
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()));
                        status(msg, 50);
                        goto endmain;
                    }
                    if (mc == 'm' || mc == 'M') {
                        noctis::toggle_audio_mute();
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        status(noctis::is_audio_muted() ? "AUDIO MUTED" : "AUDIO UNMUTED", 50);
                        goto endmain;
                    }
                    if (mc == '+' || mc == '=' || mc == ']' || mc == '.' || mc == '>') {
                        float v = noctis::step_selected_audio_category_volume(+0.05f);
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s %d%%",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()),
                                      static_cast<int>(std::round(v * 100.0f)));
                        status(msg, 50);
                        goto endmain;
                    }
                    if (mc == '-' || mc == '_' || mc == '[' || mc == ',' || mc == '<') {
                        float v = noctis::step_selected_audio_category_volume(-0.05f);
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        noctis::save_audio_settings(noctis::runtime_paths().config_dir);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "%s %d%%",
                                      noctis::audio_category_name(noctis::get_selected_audio_category()),
                                      static_cast<int>(std::round(v * 100.0f)));
                        status(msg, 50);
                        goto endmain;
                    }
                    goto endmain;
                } else if (graphics_menu_status == 3) {
                    if (mc == 9 || mc == 'v' || mc == 'V') {
                        graphics_menu_status = 1;
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 'a' || mc == 'A') {
                        graphics_menu_status = 2;
                        noctis::touch_volume_slider();
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 27) {
                        graphics_menu_status = 0;
                        goto endmain;
                    }
                    if (mc == 'i' || mc == 'I') {
                        const bool inv = noctis::toggle_mouse_inversion();
                        status(inv ? "MOUSE PITCH: INVERTED" : "MOUSE PITCH: NORMAL", 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == '-' || mc == '_') {
                        const float s = noctis::adjust_mouse_sensitivity(-0.1f);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                        status(msg, 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == '+' || mc == '=') {
                        const float s = noctis::adjust_mouse_sensitivity(+0.1f);
                        char msg[32];
                        std::snprintf(msg, sizeof(msg), "SENSITIVITY: %.1fX", s);
                        status(msg, 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 'm' || mc == 'M') {
                        option_mouse_look = noctis::cycle_mouse_look(option_mouse_look);
                        status(option_mouse_look == 0 ? "MOUSELOOK OFF" : option_mouse_look == 1 ? "MOUSELOOK ON" : "INV. Y AXIS", 50);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    if (mc == 'r' || mc == 'R') {
                        const bool r = noctis::toggle_rumble();
                        status(r ? "RUMBLE HAPTICS: ON" : "RUMBLE HAPTICS: OFF", 50);
                        if (r) noctis::trigger_gamepad_rumble(noctis::GamepadRumbleType::click, 1.0f);
                        noctis::save_controls_settings(noctis::runtime_paths().config_dir);
                        noctis::play_cockpit_button();
                        goto endmain;
                    }
                    goto endmain;
                }

                const auto snapshot_command = noctis::snapshot_action(mc, false, labstar || labplanet, false);
                if (snapshot_command == noctis::SnapshotAction::normal) {
                    snapshot(0, 1);
                    goto endmain;
                }

                if (snapshot_command == noctis::SnapshotAction::raw) {
                    snapshot(0, 0);
                    goto endmain;
                }

                if (noctis::is_roof_speed_key(mc) && !(labstar || labplanet) && !graphics_menu_status &&
                    !ip_targetting && !manual_target) {
                    noctis::toggle_timewarp();
                    roof_speed = noctis::is_timewarp_active() ? 1 : 0;
                    noctis::touch_timewarp_slider();
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), noctis::is_timewarp_active() ? "TIME %dx" : "REALTIME 1x",
                                  noctis::get_timewarp_multiplier());
                    status(msg, 50);
                    goto endmain;
                }

                if ((mc == '[' || mc == ']' || mc == '-' || mc == '+' || mc == '=') &&
                    !(labstar || labplanet) && !graphics_menu_status &&
                    !ip_targetting && !manual_target) {
                    const auto m = noctis::step_timewarp_multiplier((mc == '[' || mc == '-') ? -1 : 1);
                    noctis::touch_timewarp_slider();
                    save_display_settings_current();
                    char msg[32];
                    std::snprintf(msg, sizeof(msg), "SPEED %dx", m);
                    status(msg, 50);
                    goto endmain;
                }

                if ((mc == 'j' || mc == 'J') && !(labstar || labplanet) && !graphics_menu_status &&
                    !ip_targetting && !manual_target) {
                    active_screen = 0;
                    std::strncpy(goesnet_command, "BM_", sizeof(goesnet_command) - 1);
                    goesnet_command[sizeof(goesnet_command) - 1] = 0;
                    gnc_pos = 2;
                    run_goesnet_module();
                    status("WAYPOINT JOURNAL", 50);
                    goto endmain;
                }

                if (data) {
                    if (mc == 27) {
                        mc             = 0;
                        datasheetdelta = -2;
                        goto endmain;
                    }
                }

                if (ap_targetting) {
                    if (mc == 27) {
                        mc            = 0;
                        ap_targetting = 0;
                        ap_targetted  = 0;
                        status("CANCELLED", 50);
                        goto endmain;
                    }
                }

                if (labstar) {
                    if (mc == 27) {
                        mc               = 0;
                        labstar          = 0;
                        ap_target_previd = -1;
                        goto endmain;
                    }

                    if (mc >= 32 && mc <= 126 && labstar_char < 20) {
                        if (mc >= 'a' && mc <= 'z') {
                            mc -= 32;
                        }

                        star_label[labstar_char] = mc;
                        labstar_char++;
                        noctis::play_terminal_keystroke();
                    }

                    if (mc == 8 && labstar_char > 0) {
                        labstar_char--;
                        star_label[labstar_char] = 32;
                        noctis::play_terminal_keystroke();
                    }

                    if (mc == 13) {
                        dev_page  = 3;
                        s_command = 1;
                        dev_commands();
                    }

                    goto endmain;
                }

                if (labplanet) {
                    if (mc == 27) {
                        mc             = 0;
                        labplanet      = 0;
                        prev_planet_id = -1;
                        goto endmain;
                    }

                    if (mc >= 32 && mc <= 126 && labplanet_char < 20) {
                        if (mc >= 'a' && mc <= 'z') {
                            mc -= 32;
                        }

                        planet_label[labplanet_char] = mc;
                        labplanet_char++;
                        noctis::play_terminal_keystroke();
                    }

                    if (mc == 8 && labplanet_char > 0) {
                        labplanet_char--;
                        planet_label[labplanet_char] = 32;
                        noctis::play_terminal_keystroke();
                    }

                    if (mc == 13) {
                        dev_page  = 3;
                        s_command = 2;
                        dev_commands();
                    }

                    goto endmain;
                }

                if (targets_in_range) {
                    if (mc == 27) {
                        mc               = 0;
                        targets_in_range = 0;
                        goto endmain;
                    }

                    if (mc == 13) {
                        if (!collecting_targets && topmost_target + target_line < targets) {
                            ap_target_x = targets_table_px[topmost_target + target_line];
                            ap_target_y = targets_table_py[topmost_target + target_line];
                            ap_target_z = targets_table_pz[topmost_target + target_line];
                            extract_ap_target_infos();
                            fix_remote_target();
                        }

                        goto endmain;
                    }
                }

                if (ip_targetting) {
                    if (mc == 27) {
                        mc            = 0;
                        ip_targetted  = -1;
                        ip_targetting = 0;
                        status("CANCELLED", 50);
                        goto endmain;
                    }

                    if (mc == 8 && iptargetchar > 0) {
                        iptargetchar--;
                        iptargetstring[iptargetchar] = 0;
                        status((char *) iptargetstring, 100);
                    }

                    if (((mc >= '0' && mc <= '9') || mc == '/') && iptargetchar < 10) {
                        if (mc == '/') {
                            if (iptargetchar == 0) {
                                goto endmain;
                            }

                            ir = 0;

                            while (ir < iptargetchar) {
                                if (iptargetstring[ir] == '/') {
                                    goto endmain;
                                }

                                ir++;
                            }
                        }

                        iptargetstring[iptargetchar]     = mc;
                        iptargetstring[iptargetchar + 1] = 0;
                        iptargetchar++;
                        status((char *) iptargetstring, 100);
                    }

                    if (mc == 13) {
                        if (iptargetchar == 0) {
                            goto endmain;
                        }

                        ir = 0;

                        while (ir < iptargetchar) {
                            if (iptargetstring[ir] == '/') {
                                iptargetstring[ir] = 0;
                                iptargetmoon       = atoi((char *) iptargetstring);
                                iptargetstring[ir] = '/';

                                if (iptargetstring[ir + 1] != 0) {
                                    iptargetplanet = atoi((char *) iptargetstring + ir + 1);
                                    goto searchmoon;
                                }

                                status("NOT EXTANT", 100);
                                goto endmain;
                            }

                            ir++;
                        }

                        iptargetplanet = atoi((char *) iptargetstring);

                        if (iptargetplanet != 0 && iptargetplanet <= nearstar_nop) {
                            ip_targetted = iptargetplanet - 1;
                            fix_local_target();
                            ip_targetting = 0;
                        }

                        status("NOT EXTANT", 100);
                        goto endmain;
                    searchmoon:
                        ir = 0;

                        while (ir < nearstar_nob) {
                            if (nearstar_p_owner[ir] == iptargetplanet - 1 &&
                                nearstar_p_moonid[ir] == iptargetmoon - 1) {
                                ip_targetted = ir;
                                fix_local_target();
                                ip_targetting = 0;
                                goto endmain;
                            }

                            ir++;
                        }

                        status("NOT EXTANT", 100);
                    }

                    goto endmain;
                }

                if (manual_target) {
                    if (mc == 27) {
                        mc            = 0;
                        manual_target = 0;
                        ap_targetted  = 0;
                        status("CANCELLED", 50);
                        goto endmain;
                    }

                    if (mc == 8 && mt_string_char > 0) {
                        mt_string_char--;

                        switch (mt_coord) {
                        case 0:
                            manual_x_string[mt_string_char] = 0;
                            break;

                        case 1:
                            manual_y_string[mt_string_char] = 0;
                            break;

                        case 2:
                            manual_z_string[mt_string_char] = 0;
                            break;
                        default:
                            break;
                        }
                    }

                    if ((mc >= '0' && mc <= '9' && mt_string_char < 10) || (mc == '-' && mt_string_char == 0)) {
                        switch (mt_coord) {
                        case 0:
                            manual_x_string[mt_string_char]     = mc;
                            manual_x_string[mt_string_char + 1] = 0;
                            break;

                        case 1:
                            manual_y_string[mt_string_char]     = mc;
                            manual_y_string[mt_string_char + 1] = 0;
                            break;

                        case 2:
                            manual_z_string[mt_string_char]     = mc;
                            manual_z_string[mt_string_char + 1] = 0;
                            break;
                        default:
                            break;
                        }

                        mt_string_char++;
                    }

                    if (mc == 13) {
                        switch (mt_coord) {
                        case 0:
                            ap_target_x        = atol((char *) manual_x_string);
                            manual_y_string[0] = 0;
                            break;

                        case 1:
                            ap_target_y        = -atol((char *) manual_y_string);
                            manual_z_string[0] = 0;
                            break;

                        case 2:
                            ap_target_z = atol((char *) manual_z_string);
                            break;
                        default:
                            break;
                        }

                        mt_string_char = 0;
                        mt_coord++;

                        if (mt_coord > 2) {
                            manual_target = 0;
                            fix_remote_target();
                            ap_targetted = -1;
                        }
                    }

                    switch (mt_coord) {
                    case 0:
                        sprintf(temp_distance_buffer, "%s", manual_x_string);
                        break;

                    case 1:
                        sprintf(temp_distance_buffer, "%s", manual_y_string);
                        break;

                    case 2:
                        sprintf(temp_distance_buffer, "%s", manual_z_string);
                        break;
                    default:
                        break;
                    }

                    if (mt_coord <= 2) {
                        status(temp_distance_buffer, 100);
                    }

                    goto endmain;
                }

                if (!ap_targetting && !ip_targetting) {
                    aso_countdown     = 100;
                    const auto action = noctis::cockpit_action_for_key(mc, labstar || labplanet);
                    if (action.kind == noctis::CockpitActionKind::select_menu) {
                        sys      = action.value;
                        dev_page = 0;
                    } else if (action.kind == noctis::CockpitActionKind::run_command) {
                        s_command = action.value;
                        commands();
                    } else if (action.kind == noctis::CockpitActionKind::brighten && surlight < 63) {
                        surlight++;
                    } else if (action.kind == noctis::CockpitActionKind::dim && surlight > 10) {
                        surlight--;
                    }
                }
            }

        endmain:
            uint8_t argblarg = 5;
        }
    } else {
        mc = 0;
    }
}
