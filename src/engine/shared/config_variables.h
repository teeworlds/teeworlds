#ifndef ENGINE_SHARED_E_CONFIG_VARIABLES_H
#define ENGINE_SHARED_E_CONFIG_VARIABLES_H
#undef ENGINE_SHARED_E_CONFIG_VARIABLES_H // this file will be included several times

// TODO: remove this
#include "././game/variables.h"

//===============================  	
MACRO_CONFIG_INT(SvReconnectTime,sv_reconnect_time,5,0,9999,CFGFLAG_SERVER,"how much time between leaves and joins")
MACRO_CONFIG_INT(SvVoteKickTimeDelay,sv_vote_kick_delay,0,0,9999,CFGFLAG_SERVER,"how much time between kick votes")
MACRO_CONFIG_INT(SvVoteKickBanTime,sv_vote_kick_bantime, 300, 0, 9999, CFGFLAG_SERVER," ")
MACRO_CONFIG_INT(SvVoteMapTimeDelay,sv_vote_map_delay,0,0,9999,CFGFLAG_SERVER,"how much time between map votes")
//MACRO_CONFIG_INT(SvMaxConnections,sv_max_connections, 2, 1, 16, CFGFLAG_SERVER, "Maximum count of connection from one IP server can accept") depricated
MACRO_CONFIG_INT(SvMaxAfkTime,sv_max_afk_time, 0, 0, 9999, CFGFLAG_SERVER, "How many seconds a player is allowed to be afk, 0=disabled")	  
MACRO_CONFIG_INT(SvPauseable, sv_pauseable, 0, 0, 1, CFGFLAG_SERVER, "players can pause their char or not")  		  
MACRO_CONFIG_INT(SvCheatTime, sv_cheattime, 0, 0, 1, CFGFLAG_SERVER, "players can cheat with time or not")
MACRO_CONFIG_INT(SvHit, sv_hit, 1, 0, 1, CFGFLAG_SERVER, "players can hammer/grenade/laser one another")  		 
MACRO_CONFIG_INT(SvTunes, sv_tunes, 1, 0, 1, CFGFLAG_SERVER, "Turns Tuning On/Off")  		 
MACRO_CONFIG_INT(SvPhook, sv_phook, 1, 0, 1, CFGFLAG_SERVER, "Turns Player On/Off")  		 
MACRO_CONFIG_INT(SvNpc, sv_npc, 0, 0, 1, CFGFLAG_SERVER, "Turns NPC (No Player Collision) On/Off")  		 
MACRO_CONFIG_INT(SvEndlessDrag, sv_endless_drag, 0, 0, 1, CFGFLAG_SERVER, "Turns Endless hooking On/Off")  		 
MACRO_CONFIG_INT(SvCheats, sv_cheats, 0, 0, 1, CFGFLAG_SERVER, "Turns Cheats On/Off")  		 
MACRO_CONFIG_INT(SvAllowColorChange, sv_allow_color_change, 1, 0, 1, CFGFLAG_SERVER, "Allow color change (can block rainbowmod)")  		 
MACRO_CONFIG_INT(SvRconTries, sv_rcon_tries, 5, 0, 100, CFGFLAG_SERVER, "How Many Password Tries Before ban")  		 
MACRO_CONFIG_INT(SvRconTriesBantime, sv_rcon_tries_bantime, 300, 0, 9999, CFGFLAG_SERVER, "How Much time will the brute rcon password attacker will be banned")  		 

MACRO_CONFIG_STR(SvRconPasswordAdmin, sv_rcon_password_admin, 32, "", CFGFLAG_SERVER, "Remote console administrator password")
MACRO_CONFIG_STR(SvRconPasswordModer, sv_rcon_password_moder, 32, "", CFGFLAG_SERVER, "Remote console moderator password")  		 
MACRO_CONFIG_STR(SvRconPasswordHelper, sv_rcon_password_helper, 32, "", CFGFLAG_SERVER, "Remote console helper password")  

MACRO_CONFIG_INT(SvNetmsgLimit, sv_netmsg_limit, 0, 0, 100, CFGFLAG_SERVER, "How Many unauthed Command Tries Before ban")
MACRO_CONFIG_INT(SvNetmsgBanTime, sv_netmsg_bantime, 300, 0, 9999, CFGFLAG_SERVER, "How Much time will the unauthed rcon command spammer will be banned")

//=============================== */  

MACRO_CONFIG_STR(PlayerName, player_name, 24, "nameless tee", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Name of the player")
MACRO_CONFIG_STR(ClanName, clan_name, 32, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "(not used)")
MACRO_CONFIG_STR(Password, password, 32, "", CFGFLAG_CLIENT|CFGFLAG_SERVER, "Password to the server")
MACRO_CONFIG_STR(Logfile, logfile, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filename to log all output to")

MACRO_CONFIG_INT(ClCpuThrottle, cl_cpu_throttle, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(ClEditor, cl_editor, 0, 0, 1, CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(ClEventthread, cl_eventthread, 0, 0, 1, CFGFLAG_CLIENT, "Enables the usage of a thread to pump the events")

MACRO_CONFIG_INT(InpGrab, inp_grab, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use forceful input grabbing method")

MACRO_CONFIG_STR(BrFilterString, br_filter_string, 25, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server browser filtering string")

MACRO_CONFIG_INT(BrFilterFull, br_filter_full, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out full server in browser")
MACRO_CONFIG_INT(BrFilterEmpty, br_filter_empty, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out empty server in browser")
MACRO_CONFIG_INT(BrFilterPw, br_filter_pw, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out password protected servers in browser")
MACRO_CONFIG_INT(BrFilterPing, br_filter_ping, 999, 0, 999, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Ping to filter by in the server browser")
MACRO_CONFIG_STR(BrFilterGametype, br_filter_gametype, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Game types to filter")
MACRO_CONFIG_INT(BrFilterPure, br_filter_pure, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard servers in browser")
MACRO_CONFIG_INT(BrFilterPureMap, br_filter_pure_map, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard maps in browser")
MACRO_CONFIG_INT(BrFilterCompatversion, br_filter_compatversion, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-compatible servers in browser")

MACRO_CONFIG_INT(BrSort, br_sort, 0, 0, 256, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(BrSortOrder, br_sort_order, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(BrMaxRequests, br_max_requests, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Number of requests to use when refreshing server browser")

MACRO_CONFIG_INT(SndBufferSize, snd_buffer_size, 512, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound buffer size")
MACRO_CONFIG_INT(SndRate, snd_rate, 48000, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound mixing rate")
MACRO_CONFIG_INT(SndEnable, snd_enable, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound enable")
MACRO_CONFIG_INT(SndVolume, snd_volume, 100, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound volume")
MACRO_CONFIG_INT(SndDevice, snd_device, -1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "(deprecated) Sound device to use")

MACRO_CONFIG_INT(SndNonactiveMute, snd_nonactive_mute, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(GfxScreenWidth, gfx_screen_width, 800, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution width")
MACRO_CONFIG_INT(GfxScreenHeight, gfx_screen_height, 600, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution height")
MACRO_CONFIG_INT(GfxFullscreen, gfx_fullscreen, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fullscreen")
MACRO_CONFIG_INT(GfxAlphabits, gfx_alphabits, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Alpha bits for framebuffer  (fullscreen only)")
MACRO_CONFIG_INT(GfxColorDepth, gfx_color_depth, 24, 16, 24, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Colors bits for framebuffer (fullscreen only)")
MACRO_CONFIG_INT(GfxClear, gfx_clear, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clear screen before rendering")
MACRO_CONFIG_INT(GfxVsync, gfx_vsync, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Vertical sync")
MACRO_CONFIG_INT(GfxDisplayAllModes, gfx_display_all_modes, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(GfxTextureCompression, gfx_texture_compression, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use texture compression")
MACRO_CONFIG_INT(GfxHighDetail, gfx_high_detail, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "High detail")
MACRO_CONFIG_INT(GfxTextureQuality, gfx_texture_quality, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(GfxFsaaSamples, gfx_fsaa_samples, 0, 0, 16, CFGFLAG_SAVE|CFGFLAG_CLIENT, "FSAA Samples")
MACRO_CONFIG_INT(GfxRefreshRate, gfx_refresh_rate, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen refresh rate")
MACRO_CONFIG_INT(GfxFinish, gfx_finish, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(InpMousesens, inp_mousesens, 100, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity")

MACRO_CONFIG_STR(SvName, sv_name, 128, "DDRace Test Server", CFGFLAG_SERVER, "Server name")
MACRO_CONFIG_STR(SvBindaddr, sv_bindaddr, 128, "", CFGFLAG_SERVER, "Address to bind the server to")
MACRO_CONFIG_INT(SvPort, sv_port, 8303, 0, 0, CFGFLAG_SERVER, "Port to use for the server")
MACRO_CONFIG_INT(SvExternalPort, sv_external_port, 0, 0, 0, CFGFLAG_SERVER, "External port to report to the master servers")
MACRO_CONFIG_STR(SvMap, sv_map, 128, "Test", CFGFLAG_SERVER, "Map to use on the server")
MACRO_CONFIG_INT(SvMaxClients, sv_max_clients, 16, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients that are allowed on a server")
MACRO_CONFIG_INT(SvMaxClientsPerIP, sv_max_clients_per_ip, 2, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients with the same IP that can connect to the server")
MACRO_CONFIG_INT(SvHighBandwidth, sv_high_bandwidth, 0, 0, 1, CFGFLAG_SERVER, "Use high bandwidth mode. Doubles the bandwidth required for the server. LAN use only")
MACRO_CONFIG_INT(SvRegister, sv_register, 1, 0, 1, CFGFLAG_SERVER, "Register server with master server for public listing")

MACRO_CONFIG_INT(Debug, debug, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Debug mode")
MACRO_CONFIG_INT(DbgStress, dbg_stress, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress systems")
MACRO_CONFIG_INT(DbgStressNetwork, dbg_stress_network, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress network")
MACRO_CONFIG_INT(DbgPref, dbg_pref, 0, 0, 1, CFGFLAG_SERVER, "Performance outputs")
MACRO_CONFIG_INT(DbgGraphs, dbg_graphs, 0, 0, 1, CFGFLAG_CLIENT, "Performance graphs")
MACRO_CONFIG_INT(DbgHitch, dbg_hitch, 0, 0, 0, CFGFLAG_SERVER, "Hitch warnings")
MACRO_CONFIG_STR(DbgStressServer, dbg_stress_server, 32, "localhost", CFGFLAG_CLIENT, "Server to stress")
MACRO_CONFIG_INT(DbgResizable, dbg_resizable, 0, 0, 0, CFGFLAG_CLIENT, "Enables window resizing")
#endif
