/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_CONFIG_VARIABLES_H
#define ENGINE_SHARED_CONFIG_VARIABLES_H
#undef ENGINE_SHARED_CONFIG_VARIABLES_H // this file will be included several times

// TODO: remove this
#include "././game/variables.h"


MACRO_CONFIG_STR(PlayerName, player_name, 16, "nameless tee", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Name of the player")
MACRO_CONFIG_STR(PlayerClan, player_clan, 12, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clan of the player")
MACRO_CONFIG_INT(PlayerCountry, player_country, -1, -1, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Country of the player")
MACRO_CONFIG_STR(Password, password, 32, "", CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Password to the server")
MACRO_CONFIG_STR(Logfile, logfile, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Filename to log all output to")
MACRO_CONFIG_INT(LogfileTimestamp, logfile_timestamp, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Add a time stamp to the log file's name")
MACRO_CONFIG_INT(ConsoleOutputLevel, console_output_level, 0, 0, 2, CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Adjusts the amount of information in the console")
MACRO_CONFIG_INT(ShowConsoleWindow, show_console_window, 1, 0, 3, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Show console window (0 = never, 1 = debug, 2 = release, 3 = always")

MACRO_CONFIG_INT(ClCpuThrottle, cl_cpu_throttle, 0, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Throttles the main thread")
MACRO_CONFIG_INT(ClEditor, cl_editor, 0, 0, 1, CFGFLAG_CLIENT, "View the editor")
MACRO_CONFIG_INT(ClLoadCountryFlags, cl_load_country_flags, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Load and show country flags")

MACRO_CONFIG_INT(ClAutoDemoRecord, cl_auto_demo_record, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically record demos")
MACRO_CONFIG_INT(ClAutoDemoMax, cl_auto_demo_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically recorded demos (0 = no limit)")
MACRO_CONFIG_INT(ClAutoScreenshot, cl_auto_screenshot, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically take game over screenshot")
MACRO_CONFIG_INT(ClAutoStatScreenshot, cl_auto_statscreenshot, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically take screenshot of game statistics")
MACRO_CONFIG_INT(ClAutoScreenshotMax, cl_auto_screenshot_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically created screenshots (0 = no limit)")

MACRO_CONFIG_INT(ClShowServerBroadcast, cl_show_server_broadcast, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Show server broadcast")
MACRO_CONFIG_INT(ClColoredBroadcast, cl_colored_broadcast, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable colored server broadcasts")

MACRO_CONFIG_STR(BrFilterString, br_filter_string, 25, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server browser filtering string")

MACRO_CONFIG_INT(BrSort, br_sort, 4, 0, 256, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sort criterion for the server browser")
MACRO_CONFIG_INT(BrSortOrder, br_sort_order, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sort order in the server browser")
MACRO_CONFIG_INT(BrMaxRequests, br_max_requests, 25, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Number of requests to use when refreshing server browser")

MACRO_CONFIG_INT(SndBufferSize, snd_buffer_size, 512, 128, 32768, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound buffer size")
MACRO_CONFIG_INT(SndRate, snd_rate, 48000, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound mixing rate")
MACRO_CONFIG_INT(SndEnable, snd_enable, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable sounds")
MACRO_CONFIG_INT(SndInit, snd_init, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Initialize sound systems")
MACRO_CONFIG_INT(SndMusic, snd_enable_music, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Play background music")
MACRO_CONFIG_INT(SndVolume, snd_volume, 100, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound volume")
MACRO_CONFIG_INT(SndNonactiveMute, snd_nonactive_mute, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mute the application when not active")
MACRO_CONFIG_INT(SndAsyncLoading, snd_async_loading, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Load sound files threaded")

MACRO_CONFIG_INT(GfxScreen, gfx_screen, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen index")
MACRO_CONFIG_INT(GfxScreenWidth, gfx_screen_width, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution width")
MACRO_CONFIG_INT(GfxScreenHeight, gfx_screen_height, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution height")
MACRO_CONFIG_INT(GfxBorderless, gfx_borderless, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Borderless window (not to be used with fullscreen)")
MACRO_CONFIG_INT(GfxFullscreen, gfx_fullscreen, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fullscreen")
MACRO_CONFIG_INT(GfxAlphabits, gfx_alphabits, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Alpha bits for framebuffer (fullscreen only)")
MACRO_CONFIG_INT(GfxClear, gfx_clear, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clear screen before rendering")
MACRO_CONFIG_INT(GfxVsync, gfx_vsync, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Vertical sync")
MACRO_CONFIG_INT(GfxDisplayAllModes, gfx_display_all_modes, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "List non-supported display modes")
MACRO_CONFIG_INT(GfxHighdpi, gfx_highdpi, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use high dpi mode if available")
MACRO_CONFIG_INT(GfxTextureCompression, gfx_texture_compression, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use texture compression")
MACRO_CONFIG_INT(GfxHighDetail, gfx_high_detail, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "High detail")
MACRO_CONFIG_INT(GfxTextureQuality, gfx_texture_quality, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Don't scale textures down")
MACRO_CONFIG_INT(GfxFsaaSamples, gfx_fsaa_samples, 0, 0, 16, CFGFLAG_SAVE|CFGFLAG_CLIENT, "FSAA Samples")
MACRO_CONFIG_INT(GfxFinish, gfx_finish, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Wait till the gpu finished the current frame before starting the new one")
MACRO_CONFIG_INT(GfxAsyncRender, gfx_asyncrender, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Do rendering async from the the update")
MACRO_CONFIG_INT(GfxMaxFps, gfx_maxfps, 144, 30, 2000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum fps (when limit fps is enabled)")
MACRO_CONFIG_INT(GfxLimitFps, gfx_limitfps, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Limit fps")
MACRO_CONFIG_INT(GfxUseX11XRandRWM, gfx_use_x11xrandr_wm, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Let SDL use the X11 XRandR window manager")

MACRO_CONFIG_INT(InpGrab, inp_grab, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Disable OS mouse settings such as mouse acceleration, use raw mouse input mode")
MACRO_CONFIG_INT(InpMousesens, inp_mousesens, 100, 1, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Ingame mouse sensitivity")

MACRO_CONFIG_INT(JoystickEnable , joystick_enable, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable joystick")
MACRO_CONFIG_INT(JoystickSens, joystick_sens, 100, 1, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Joystick sensitivity")
MACRO_CONFIG_INT(JoystickX, joystick_x, 0, 0, 6, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Joystick axis that controls X axis of cursor")
MACRO_CONFIG_INT(JoystickY, joystick_y, 1, 0, 6, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Joystick axis that controls Y axis of cursor")
MACRO_CONFIG_INT(JoystickTolerance, joystick_tolerance, 5, 0, 50, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Joystick Axis tolerance to account for jitter")

MACRO_CONFIG_STR(SvName, sv_name, 128, "unnamed server", CFGFLAG_SAVE|CFGFLAG_SERVER, "Server name")
MACRO_CONFIG_STR(SvHostname, sv_hostname, 128, "", CFGFLAG_SAVE|CFGFLAG_SERVER, "Server hostname")
MACRO_CONFIG_STR(Bindaddr, bindaddr, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER|CFGFLAG_MASTER, "Address to bind the client/server to")
MACRO_CONFIG_INT(SvPort, sv_port, 8303, 0, 0, CFGFLAG_SAVE|CFGFLAG_SERVER, "Port to use for the server")
MACRO_CONFIG_INT(SvExternalPort, sv_external_port, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_SERVER, "External port to report to the master servers")
MACRO_CONFIG_STR(SvMap, sv_map, 128, "dm1", CFGFLAG_SAVE|CFGFLAG_SERVER, "Map to use on the server")
MACRO_CONFIG_INT(SvMaxClients, sv_max_clients, 8, 1, MAX_CLIENTS, CFGFLAG_SAVE|CFGFLAG_SERVER, "Maximum number of clients that are allowed on a server")
MACRO_CONFIG_INT(SvMaxClientsPerIP, sv_max_clients_per_ip, 4, 1, MAX_CLIENTS, CFGFLAG_SAVE|CFGFLAG_SERVER, "Maximum number of clients with the same IP that can connect to the server")
MACRO_CONFIG_INT(SvMapDownloadSpeed, sv_map_download_speed, 2, 1, 16, CFGFLAG_SAVE|CFGFLAG_SERVER, "Number of map data packages a client gets on each request")
MACRO_CONFIG_INT(SvHighBandwidth, sv_high_bandwidth, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_SERVER, "Use high bandwidth mode. Doubles the bandwidth required for the server. LAN use only")
MACRO_CONFIG_INT(SvRegister, sv_register, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_SERVER, "Register server with master server for public listing")
MACRO_CONFIG_STR(SvRconPassword, sv_rcon_password, 32, "", CFGFLAG_SAVE|CFGFLAG_SERVER, "Remote console password (full access)")
MACRO_CONFIG_STR(SvRconModPassword, sv_rcon_mod_password, 32, "", CFGFLAG_SAVE|CFGFLAG_SERVER, "Remote console password for moderators (limited access)")
MACRO_CONFIG_INT(SvRconMaxTries, sv_rcon_max_tries, 3, 0, 100, CFGFLAG_SAVE|CFGFLAG_SERVER, "Maximum number of tries for remote console authentication")
MACRO_CONFIG_INT(SvRconBantime, sv_rcon_bantime, 5, 0, 1440, CFGFLAG_SAVE|CFGFLAG_SERVER, "The time a client gets banned if remote console authentication fails. 0 makes it just use kick")
MACRO_CONFIG_INT(SvAutoDemoRecord, sv_auto_demo_record, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_SERVER, "Automatically record demos")
MACRO_CONFIG_INT(SvAutoDemoMax, sv_auto_demo_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_SERVER, "Maximum number of automatically recorded demos (0 = no limit)")

MACRO_CONFIG_STR(EcBindaddr, ec_bindaddr, 128, "localhost", CFGFLAG_SAVE|CFGFLAG_ECON, "Address to bind the external console to. Anything but 'localhost' is dangerous")
MACRO_CONFIG_INT(EcPort, ec_port, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_ECON, "Port to use for the external console")
MACRO_CONFIG_STR(EcPassword, ec_password, 32, "", CFGFLAG_SAVE|CFGFLAG_ECON, "External console password")
MACRO_CONFIG_INT(EcBantime, ec_bantime, 0, 0, 1440, CFGFLAG_SAVE|CFGFLAG_ECON, "The time a client gets banned if econ authentication fails. 0 just closes the connection")
MACRO_CONFIG_INT(EcAuthTimeout, ec_auth_timeout, 30, 1, 120, CFGFLAG_SAVE|CFGFLAG_ECON, "Time in seconds before the the econ authentification times out")
MACRO_CONFIG_INT(EcOutputLevel, ec_output_level, 1, 0, 2, CFGFLAG_SAVE|CFGFLAG_ECON, "Adjusts the amount of information in the external console")

MACRO_CONFIG_INT(Debug, debug, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Debug mode")
MACRO_CONFIG_INT(DbgStress, dbg_stress, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress systems")
MACRO_CONFIG_INT(DbgStressNetwork, dbg_stress_network, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress network")
MACRO_CONFIG_INT(DbgPref, dbg_pref, 0, 0, 1, CFGFLAG_SERVER, "Performance outputs")
MACRO_CONFIG_INT(DbgGraphs, dbg_graphs, 0, 0, 1, CFGFLAG_CLIENT, "Performance graphs")
MACRO_CONFIG_INT(DbgHitch, dbg_hitch, 0, 0, 0, CFGFLAG_SERVER, "Hitch warnings")
MACRO_CONFIG_STR(DbgStressServer, dbg_stress_server, 32, "localhost", CFGFLAG_CLIENT, "Server to stress")
MACRO_CONFIG_INT(DbgResizable, dbg_resizable, 0, 0, 0, CFGFLAG_CLIENT, "Enables window resizing")

#endif
