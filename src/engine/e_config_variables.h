/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

/* TODO: remove this */
#include "../game/variables.hpp"


MACRO_CONFIG_STR(player_name, 32, "nameless tee", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Name of the player")
MACRO_CONFIG_STR(clan_name, 32, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "(not used)")
MACRO_CONFIG_STR(password, 32, "", CFGFLAG_CLIENT, "Password to the server")
MACRO_CONFIG_STR(logfile, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filename to log all output to")

MACRO_CONFIG_INT(cl_cpu_throttle, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(cl_editor, 0, 0, 1, CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(cl_eventthread, 0, 0, 1, CFGFLAG_CLIENT, "Enables the usage of a thread to pump the events")

MACRO_CONFIG_INT(inp_grab, 0, 0, 1, CFGFLAG_CLIENT, "Use forceful input grabbing method")

MACRO_CONFIG_STR(b_filter_string, 64, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server browser filtering string")

MACRO_CONFIG_INT(b_filter_full, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out full server in browser")
MACRO_CONFIG_INT(b_filter_empty, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out empty server in browser")
MACRO_CONFIG_INT(b_filter_pw, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out password protected servers in browser")
MACRO_CONFIG_INT(b_filter_ping, 999, 0, 999, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Ping to filter by in the server browser")
MACRO_CONFIG_STR(b_filter_gametype, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Game types to filter")
MACRO_CONFIG_INT(b_filter_pure, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard servers in browser")
MACRO_CONFIG_INT(b_filter_pure_map, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard maps in browser")
MACRO_CONFIG_INT(b_filter_compatversion, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-compatible servers in browser")

MACRO_CONFIG_INT(b_sort, 0, 0, 256, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(b_sort_order, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(b_max_requests, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Number of requests to use when refreshing server browser")

MACRO_CONFIG_INT(snd_buffer_size, 512, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound buffer size")
MACRO_CONFIG_INT(snd_rate, 48000, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound mixing rate")
MACRO_CONFIG_INT(snd_enable, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound enable")
MACRO_CONFIG_INT(snd_volume, 100, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound volume")
MACRO_CONFIG_INT(snd_device, -1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "(deprecated) Sound device to use")

MACRO_CONFIG_INT(snd_nonactive_mute, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(gfx_screen_width, 800, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution width")
MACRO_CONFIG_INT(gfx_screen_height, 600, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution height")
MACRO_CONFIG_INT(gfx_fullscreen, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fullscreen")
MACRO_CONFIG_INT(gfx_alphabits, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Alpha bits for framebuffer  (fullscreen only)")
MACRO_CONFIG_INT(gfx_color_depth, 24, 16, 24, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Colors bits for framebuffer (fullscreen only)")
MACRO_CONFIG_INT(gfx_clear, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clear screen before rendering")
MACRO_CONFIG_INT(gfx_vsync, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Vertical sync")
MACRO_CONFIG_INT(gfx_display_all_modes, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(gfx_texture_compression, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use texture compression")
MACRO_CONFIG_INT(gfx_high_detail, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "High detail")
MACRO_CONFIG_INT(gfx_texture_quality, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(gfx_fsaa_samples, 0, 0, 16, CFGFLAG_SAVE|CFGFLAG_CLIENT, "FSAA Samples")
MACRO_CONFIG_INT(gfx_refresh_rate, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen refresh rate")
MACRO_CONFIG_INT(gfx_finish, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(inp_mousesens, 100, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity")

MACRO_CONFIG_STR(sv_name, 128, "unnamed server", CFGFLAG_SERVER, "Server name")
MACRO_CONFIG_STR(sv_bindaddr, 128, "", CFGFLAG_SERVER, "Address to bind the server to")
MACRO_CONFIG_INT(sv_port, 8303, 0, 0, CFGFLAG_SERVER, "Port to use for the server")
MACRO_CONFIG_INT(sv_external_port, 0, 0, 0, CFGFLAG_SERVER, "External port to report to the master servers")
MACRO_CONFIG_STR(sv_map, 128, "dm1", CFGFLAG_SERVER, "Map to use on the server")
MACRO_CONFIG_INT(sv_max_clients, 8, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients that are allowed on a server")
MACRO_CONFIG_INT(sv_high_bandwidth, 0, 0, 1, CFGFLAG_SERVER, "Use high bandwidth mode. Doubles the bandwidth required for the server. LAN use only")
MACRO_CONFIG_INT(sv_register, 1, 0, 1, CFGFLAG_SERVER, "Register server with master server for public listing")
MACRO_CONFIG_STR(sv_rcon_password, 32, "", CFGFLAG_SERVER, "Remote console password")
MACRO_CONFIG_INT(sv_map_reload, 0, 0, 1, CFGFLAG_SERVER, "Reload the current map")

MACRO_CONFIG_INT(debug, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Debug mode")
MACRO_CONFIG_INT(dbg_stress, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress systems")
MACRO_CONFIG_INT(dbg_stress_network, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress network")
MACRO_CONFIG_INT(dbg_pref, 0, 0, 1, CFGFLAG_SERVER, "Performance outputs")
MACRO_CONFIG_INT(dbg_graphs, 0, 0, 1, CFGFLAG_CLIENT, "Performance graphs")
MACRO_CONFIG_INT(dbg_hitch, 0, 0, 0, CFGFLAG_SERVER, "Hitch warnings")
MACRO_CONFIG_STR(dbg_stress_server, 32, "localhost", CFGFLAG_CLIENT, "Server to stress")
MACRO_CONFIG_INT(dbg_resizable, 0, 0, 0, CFGFLAG_CLIENT, "Enables window resizing")
