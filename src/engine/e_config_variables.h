/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "../game/g_variables.h"


MACRO_CONFIG_STR(player_name, 32, "nameless tee")
MACRO_CONFIG_STR(clan_name, 32, "")
MACRO_CONFIG_STR(password, 32, "")
MACRO_CONFIG_STR(rcon_password, 32, "")

MACRO_CONFIG_INT(cl_cpu_throttle, 0, 0, 1)
/*MACRO_CONFIG_STR(cl_connect, 32, "")*/
MACRO_CONFIG_INT(cl_editor, 0, 0, 1)

MACRO_CONFIG_STR(b_filter_string, 64, "")

MACRO_CONFIG_INT(b_filter_full, 0, 0, 1)
MACRO_CONFIG_INT(b_filter_empty, 0, 0, 1)
MACRO_CONFIG_INT(b_filter_pw, 0, 0, 1)
MACRO_CONFIG_INT(b_sort, 0, 0, 256)
MACRO_CONFIG_INT(b_sort_order, 0, 0, 1)
MACRO_CONFIG_INT(b_max_requests, 10, 0, 1000)

MACRO_CONFIG_INT(snd_rate, 48000, 0, 0)
MACRO_CONFIG_INT(snd_enable, 1, 0, 1)
MACRO_CONFIG_INT(snd_volume, 100, 0, 100)
MACRO_CONFIG_INT(snd_device, -1, 0, 0)

MACRO_CONFIG_INT(gfx_screen_width, 800, 0, 0)
MACRO_CONFIG_INT(gfx_screen_height, 600, 0, 0)
MACRO_CONFIG_INT(gfx_fullscreen, 1, 0, 1)
MACRO_CONFIG_INT(gfx_color_depth, 24, 16, 24)
MACRO_CONFIG_INT(gfx_vsync, 1, 0, 1)
MACRO_CONFIG_INT(gfx_display_all_modes, 0, 0, 1)
MACRO_CONFIG_INT(gfx_texture_compression, 0, 0, 1)
MACRO_CONFIG_INT(gfx_high_detail, 1, 0, 1)
MACRO_CONFIG_INT(gfx_texture_quality, 1, 0, 1)
MACRO_CONFIG_INT(gfx_fsaa_samples, 0, 0, 16)
MACRO_CONFIG_INT(gfx_refresh_rate, 0, 0, 0)
MACRO_CONFIG_INT(gfx_debug_resizable, 0, 0, 0)

MACRO_CONFIG_INT(key_screenshot, 267, 32, 512)
MACRO_CONFIG_INT(inp_mousesens, 100, 5, 100000)

MACRO_CONFIG_STR(masterserver, 128, "master.teewars.com")

MACRO_CONFIG_STR(sv_name, 128, "unnamed server")
MACRO_CONFIG_STR(sv_bindaddr, 128, "")
MACRO_CONFIG_INT(sv_port, 8303, 0, 0)
MACRO_CONFIG_INT(sv_external_port, 0, 0, 0)
MACRO_CONFIG_INT(sv_sendheartbeats, 1, 0, 1)
MACRO_CONFIG_STR(sv_map, 128, "dm1")
MACRO_CONFIG_INT(sv_map_reload, 0, 0, 1)
MACRO_CONFIG_INT(sv_max_clients, 8, 1, 12)
MACRO_CONFIG_INT(sv_high_bandwidth, 0, 0, 1)
MACRO_CONFIG_INT(sv_status, 0, 0, 1)

MACRO_CONFIG_INT(debug, 0, 0, 1)
MACRO_CONFIG_INT(dbg_stress, 0, 0, 0)
MACRO_CONFIG_INT(dbg_pref, 0, 0, 1)
MACRO_CONFIG_INT(dbg_hitch, 0, 0, 0)
MACRO_CONFIG_STR(dbg_stress_server, 32, "localhost")

