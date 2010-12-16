/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_E_CONFIG_VARIABLES_H
#define ENGINE_SHARED_E_CONFIG_VARIABLES_H
#undef ENGINE_SHARED_E_CONFIG_VARIABLES_H // this file will be included several times

// TODO: remove this
#include "././game/variables.h"


MACRO_CONFIG_STR(PlayerName, player_name, 24, "nameless tee", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Name of the player", -1)
MACRO_CONFIG_STR(ClanName, clan_name, 32, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "(not used)", -1)
MACRO_CONFIG_STR(Password, password, 32, "", CFGFLAG_CLIENT|CFGFLAG_SERVER, "Password to the server", 3)
MACRO_CONFIG_STR(Logfile, logfile, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Filename to log all output to", 3)
MACRO_CONFIG_INT(ConsoleOutputLevel, console_output_level, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Adjusts the amount of information in the console", 3)

MACRO_CONFIG_INT(ClAutoDemoRecord, cl_auto_demo_record, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically record demos", -1)

MACRO_CONFIG_INT(ClCpuThrottle, cl_cpu_throttle, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(ClEditor, cl_editor, 0, 0, 1, CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(ClAutoDemoMax, cl_auto_demo_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically recorded demos (0 = no limit)", -1)
MACRO_CONFIG_INT(ClAutoScreenshot, cl_auto_screenshot, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically take game over screenshot", -1)
MACRO_CONFIG_INT(ClAutoScreenshotMax, cl_auto_screenshot_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically created screenshots (0 = no limit)", -1)

MACRO_CONFIG_INT(ClEventthread, cl_eventthread, 0, 0, 1, CFGFLAG_CLIENT, "Enables the usage of a thread to pump the events", -1)

MACRO_CONFIG_INT(ClShowIds, cl_show_ids, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Whether to show client ids in scoreboard", -1)

MACRO_CONFIG_INT(InpGrab, inp_grab, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use forceful input grabbing method", -1)

MACRO_CONFIG_STR(BrFilterString, br_filter_string, 25, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server browser filtering string", -1)

MACRO_CONFIG_INT(BrFilterFull, br_filter_full, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out full server in browser", -1)
MACRO_CONFIG_INT(BrFilterEmpty, br_filter_empty, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out empty server in browser", -1)
MACRO_CONFIG_INT(BrFilterPw, br_filter_pw, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out password protected servers in browser", -1)
MACRO_CONFIG_INT(BrFilterPing, br_filter_ping, 999, 0, 999, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Ping to filter by in the server browser", -1)
MACRO_CONFIG_INT(BrFilterGametypeStrict, br_filter_gametype_strict, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter gametype strict", -1)
MACRO_CONFIG_STR(BrFilterGametype, br_filter_gametype, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Game types to filter", -1)
MACRO_CONFIG_INT(BrFilterPure, br_filter_pure, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard servers in browser", -1)
MACRO_CONFIG_INT(BrFilterPureMap, br_filter_pure_map, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard maps in browser", -1)
MACRO_CONFIG_INT(BrFilterCompatversion, br_filter_compatversion, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-compatible servers in browser", -1)

MACRO_CONFIG_INT(BrSort, br_sort, 0, 0, 256, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(BrSortOrder, br_sort_order, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(BrMaxRequests, br_max_requests, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Number of requests to use when refreshing server browser", -1)

MACRO_CONFIG_INT(SndBufferSize, snd_buffer_size, 512, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound buffer size", -1)
MACRO_CONFIG_INT(SndRate, snd_rate, 48000, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound mixing rate", -1)
MACRO_CONFIG_INT(SndEnable, snd_enable, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound enable", -1)
MACRO_CONFIG_INT(SndVolume, snd_volume, 100, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound volume", -1)
MACRO_CONFIG_INT(SndDevice, snd_device, -1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "(deprecated) Sound device to use", -1)

MACRO_CONFIG_INT(SndNonactiveMute, snd_nonactive_mute, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)

MACRO_CONFIG_INT(GfxScreenWidth, gfx_screen_width, 800, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution width", -1)
MACRO_CONFIG_INT(GfxScreenHeight, gfx_screen_height, 600, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution height", -1)
MACRO_CONFIG_INT(GfxFullscreen, gfx_fullscreen, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fullscreen", -1)
MACRO_CONFIG_INT(GfxAlphabits, gfx_alphabits, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Alpha bits for framebuffer  (fullscreen only)", -1)
MACRO_CONFIG_INT(GfxColorDepth, gfx_color_depth, 24, 16, 24, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Colors bits for framebuffer (fullscreen only)", -1)
MACRO_CONFIG_INT(GfxClear, gfx_clear, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clear screen before rendering", -1)
MACRO_CONFIG_INT(GfxVsync, gfx_vsync, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Vertical sync", -1)
MACRO_CONFIG_INT(GfxDisplayAllModes, gfx_display_all_modes, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(GfxTextureCompression, gfx_texture_compression, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use texture compression", -1)
MACRO_CONFIG_INT(GfxHighDetail, gfx_high_detail, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "High detail", -1)
MACRO_CONFIG_INT(GfxTextureQuality, gfx_texture_quality, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(GfxFsaaSamples, gfx_fsaa_samples, 0, 0, 16, CFGFLAG_SAVE|CFGFLAG_CLIENT, "FSAA Samples", -1)
MACRO_CONFIG_INT(GfxRefreshRate, gfx_refresh_rate, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen refresh rate", -1)
MACRO_CONFIG_INT(GfxFinish, gfx_finish, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "", -1)

MACRO_CONFIG_INT(InpMousesens, inp_mousesens, 100, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity", -1)

MACRO_CONFIG_STR(SvName, sv_name, 128, "DDRace Test Trunk 0.5 Server", CFGFLAG_SERVER, "Server name", 3)
MACRO_CONFIG_STR(SvBindaddr, sv_bindaddr, 128, "", CFGFLAG_SERVER, "Address to bind the server to", 3)
MACRO_CONFIG_INT(SvPort, sv_port, 8303, 0, 0, CFGFLAG_SERVER, "Port to use for the server", 3)
MACRO_CONFIG_INT(SvExternalPort, sv_external_port, 0, 0, 0, CFGFLAG_SERVER, "External port to report to the master servers", 3)
MACRO_CONFIG_STR(SvMap, sv_map, 128, "Test", CFGFLAG_SERVER, "Map to use on the server", 3)
MACRO_CONFIG_INT(SvMaxClients, sv_max_clients, 16, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients that are allowed on a server", 3)
MACRO_CONFIG_INT(SvMaxClientsPerIP, sv_max_clients_per_ip, 2, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients with the same IP that can connect to the server", 3)
MACRO_CONFIG_INT(SvHighBandwidth, sv_high_bandwidth, 0, 0, 1, CFGFLAG_SERVER, "Use high bandwidth mode. Doubles the bandwidth required for the server. LAN use only", 3)
MACRO_CONFIG_INT(SvRegister, sv_register, 1, 0, 1, CFGFLAG_SERVER, "Register server with master server for public listing", 3)
/*MACRO_CONFIG_STR(SvRconPassword, sv_rcon_password, 32, "", CFGFLAG_SERVER, "Remote console password")*/
MACRO_CONFIG_INT(SvRconMaxTries, sv_rcon_max_tries, 3, 0, 100, CFGFLAG_SERVER, "Maximum number of tries for remote console authentication", 3)
MACRO_CONFIG_INT(SvRconBantime, sv_rcon_bantime, 300, 0, 9999, CFGFLAG_SERVER, "The time a client gets banned if remote console authentication fails. 0 makes it just use kick", 3)

MACRO_CONFIG_INT(Debug, debug, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Debug mode", 3)
MACRO_CONFIG_INT(DbgStress, dbg_stress, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress systems", 3)
MACRO_CONFIG_INT(DbgStressNetwork, dbg_stress_network, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress network", 3)
MACRO_CONFIG_INT(DbgPref, dbg_pref, 0, 0, 1, CFGFLAG_SERVER, "Performance outputs", 3)
MACRO_CONFIG_INT(DbgGraphs, dbg_graphs, 0, 0, 1, CFGFLAG_CLIENT, "Performance graphs", -1)
MACRO_CONFIG_INT(DbgHitch, dbg_hitch, 0, 0, 0, CFGFLAG_SERVER, "Hitch warnings", 3)
MACRO_CONFIG_STR(DbgStressServer, dbg_stress_server, 32, "localhost", CFGFLAG_CLIENT, "Server to stress", -1)
MACRO_CONFIG_INT(DbgResizable, dbg_resizable, 0, 0, 0, CFGFLAG_CLIENT, "Enables window resizing", -1)

//DDRace
MACRO_CONFIG_STR(SvWelcome, sv_welcome, 64, "", CFGFLAG_SERVER, "Message that will be displayed to players who join the server", 3)
MACRO_CONFIG_STR(SvBroadcast, sv_broadcast, 64, "DDRace.info Trunk 0.5", CFGFLAG_SERVER, "The broadcasting message", 3)
MACRO_CONFIG_INT(SvReservedSlots, sv_reserved_slots, 0, 0, 16, CFGFLAG_SERVER, "The number of slots that are reserved for special players", 3)
MACRO_CONFIG_STR(SvReservedSlotsPass, sv_reserved_slots_pass, 32, "", CFGFLAG_SERVER, "The password that is required to use a reserved slot", 3)
MACRO_CONFIG_STR(SvRconPasswordAdmin, sv_admin_pass, 32, "", CFGFLAG_SERVER, "Remote console administrator password", 4)
MACRO_CONFIG_STR(SvRconPasswordModer, sv_mod_pass, 32, "", CFGFLAG_SERVER, "Remote console moderator password", 4)
MACRO_CONFIG_STR(SvRconPasswordHelper, sv_helper_pass, 32, "", CFGFLAG_SERVER, "Remote console helper password", 4)
MACRO_CONFIG_INT(SvHit, sv_hit, 1, 0, 1, CFGFLAG_SERVER, "Whether players can hammer/grenade/laser eachother or not", 4)
MACRO_CONFIG_INT(SvEndlessDrag, sv_endless_drag, 0, 0, 1, CFGFLAG_SERVER, "Turns endless hooking on/off", 4)

MACRO_CONFIG_INT(SvCheats, sv_cheats, 0, 0, 1, CFGFLAG_SERVER, "Turns cheats on/off", 4)
MACRO_CONFIG_INT(SvCheatTime, sv_cheattime, 0, 0, 1, CFGFLAG_SERVER, "Whether the time of players will be stopped on cheating or not", 4)
MACRO_CONFIG_INT(SvEndlessSuperHook, sv_endless_super_hook, 0, 0, 1, CFGFLAG_SERVER, "Endless hook for super players on/off", 4)

//MACRO_CONFIG_INT(SvAllowColorChange, sv_allow_color_change, 1, 0, 1, CFGFLAG_SERVER, "Whether color change is allowed (to block rainbow mod)", 3)
MACRO_CONFIG_INT(SvHideScore, sv_hide_score, 0, 0, 1, CFGFLAG_SERVER, "Whether players scores will be announced or not", 4)
MACRO_CONFIG_INT(SvTimer, sv_timer, 0, 0, 1, CFGFLAG_SERVER, "Whether timer commands are allowed or not", 3)
MACRO_CONFIG_INT(SvPauseable, sv_pauseable, 1, 0, 1, CFGFLAG_SERVER, "Whether players can pause their char or not", 3)
MACRO_CONFIG_INT(SvPauseTime, sv_pause_time, 0, 0, 1, CFGFLAG_SERVER, "Whether '/pause' and 'sv_max_dc_restore' pauses the time of player or not", 3)

MACRO_CONFIG_INT(SvEmotionalTees, sv_emotional_tees, 1, 0, 1, CFGFLAG_SERVER, "Whether eye change of tees is enabled or not", 3)
MACRO_CONFIG_INT(SvEmoticonDelay, sv_emoticon_delay, 3, 0, 9999, CFGFLAG_SERVER, "The time in seconds between over-head emoticons", 3)

MACRO_CONFIG_INT(SvChatDelay, sv_chat_delay, 1, 0, 9999, CFGFLAG_SERVER, "The time in seconds between chat messages", 3)
MACRO_CONFIG_INT(SvTeamChangeDelay, sv_team_change_delay, 3, 0, 9999, CFGFLAG_SERVER, "The time in seconds between team changes (spectator/in game)", 3)
MACRO_CONFIG_INT(SvInfoChangeDelay, sv_info_change_delay, 5, 0, 9999, CFGFLAG_SERVER, "The time in seconds between info changes (name/skin/color), to avoid ranbow mod set this to a very high time", 3)
MACRO_CONFIG_INT(SvVoteMapTimeDelay, sv_vote_map_delay,0,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between map votes", 3)
MACRO_CONFIG_INT(SvVoteDelay, sv_vote_delay, 3, 0, 9999, CFGFLAG_SERVER, "The time in seconds between any vote", 3)
MACRO_CONFIG_INT(SvVoteKickTimeDelay, sv_vote_kick_delay,0,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between kick votes", 3)
MACRO_CONFIG_INT(SvKillDelay, sv_kill_delay,3,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between kills", 3)
MACRO_CONFIG_INT(SvSuicidePenalty, sv_suicide_penalty,0,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between kill or /kills and respawn", 3)

MACRO_CONFIG_INT(SvVotePercentage, sv_vote_percentage, 50, 1, 100, CFGFLAG_SERVER, "The percent of people that need to agree or deny for the vote to succeed/fail", 4)
MACRO_CONFIG_INT(SvShotgunBulletSound, sv_shotgun_bullet_sound, 0, 0, 1, CFGFLAG_SERVER, "Crazy shotgun bullet sound on/off", 3)

MACRO_CONFIG_INT(SvCheckpointSave, sv_checkpoint_save, 1, 0, 1, CFGFLAG_SERVER, "Whether to save checkpoint times to the score file", 3)
MACRO_CONFIG_STR(SvScoreFolder, sv_score_folder, 32, "records", CFGFLAG_SERVER, "Folder to save score files to", 3)


#if defined(CONF_SQL)
MACRO_CONFIG_INT(SvUseSQL, sv_use_sql, 0, 0, 1, CFGFLAG_SERVER, "Enables SQL DB instead of record file", 4)
MACRO_CONFIG_STR(SvSqlUser, sv_sql_user, 32, "nameless", CFGFLAG_SERVER, "SQL User", 4)
MACRO_CONFIG_STR(SvSqlPw, sv_sql_pw, 32, "tee", CFGFLAG_SERVER, "SQL Password", 4)
MACRO_CONFIG_STR(SvSqlIp, sv_sql_ip, 32, "127.0.0.1", CFGFLAG_SERVER, "SQL Database IP", 4)
MACRO_CONFIG_INT(SvSqlPort, sv_sql_port, 3306, 0, 65535, CFGFLAG_SERVER, "SQL Database port", 4)
MACRO_CONFIG_STR(SvSqlDatabase, sv_sql_database, 16, "teeworlds", CFGFLAG_SERVER, "SQL Database name", 4)
MACRO_CONFIG_STR(SvSqlPrefix, sv_sql_prefix, 16, "record", CFGFLAG_SERVER, "SQL Database table prefix", 4)
#endif

MACRO_CONFIG_INT(SvDDRaceRules, sv_ddrace_rules, 1, 0, 1, CFGFLAG_SERVER, "Whether the default mod rules are displayed or not", 4)
MACRO_CONFIG_STR(SvRulesLine1, sv_rules_line1, 40, "", CFGFLAG_SERVER, "Rules line 1", 4)
MACRO_CONFIG_STR(SvRulesLine2, sv_rules_line2, 40, "", CFGFLAG_SERVER, "Rules line 2", 4)
MACRO_CONFIG_STR(SvRulesLine3, sv_rules_line3, 40, "", CFGFLAG_SERVER, "Rules line 3", 4)
MACRO_CONFIG_STR(SvRulesLine4, sv_rules_line4, 40, "", CFGFLAG_SERVER, "Rules line 4", 4)
MACRO_CONFIG_STR(SvRulesLine5, sv_rules_line5, 40, "", CFGFLAG_SERVER, "Rules line 5", 4)
MACRO_CONFIG_STR(SvRulesLine6, sv_rules_line6, 40, "", CFGFLAG_SERVER, "Rules line 6", 4)
MACRO_CONFIG_STR(SvRulesLine7, sv_rules_line7, 40, "", CFGFLAG_SERVER, "Rules line 7", 4)
MACRO_CONFIG_STR(SvRulesLine8, sv_rules_line8, 40, "", CFGFLAG_SERVER, "Rules line 8", 4)
MACRO_CONFIG_STR(SvRulesLine9, sv_rules_line9, 40, "", CFGFLAG_SERVER, "Rules line 9", 4)
MACRO_CONFIG_STR(SvRulesLine10, sv_rules_line10, 40, "", CFGFLAG_SERVER, "Rules line 10", 4)
//MACRO_CONFIG_INT(SvReconnectTime, sv_reconnect_time,5,0,9999,CFGFLAG_SERVER, "The time in seconds between leaves and joins of clients with the same ip", 3)

MACRO_CONFIG_INT(SvTeam, sv_team, 0, -1, 1, CFGFLAG_SERVER, "Teams configuration", 4)
MACRO_CONFIG_INT(SvTeamStrict, sv_team_strict, 0, 0, 1, CFGFLAG_SERVER, "Kill or not all team if someone left game in team", 4)

MACRO_CONFIG_STR(SvAnnouncementFileName, sv_announcement_filename, 24, "announcement.txt", CFGFLAG_SERVER, "file which will have the announcement, each one at a line", 3)
MACRO_CONFIG_INT(SvAnnouncementInterval, sv_announcement_interval, 30, 15, 9999, CFGFLAG_SERVER, "time(minutes) in which the announcement will be displayed from the announcement file", 3)
MACRO_CONFIG_INT(SvAnnouncementRandom, sv_announcement_random, 1, 0, 1, CFGFLAG_SERVER, "Whether announcements are sequential or random", 3)

MACRO_CONFIG_INT(SvOldLaser, sv_old_laser, 0, 0, 1, CFGFLAG_SERVER, "Whether lasers can hit you if you shot them and that they pull you towards the bounce origin (0 for DDRace Beta) or lasers can't hit you if you shot them, and they pull others towards the shooter", 4)
MACRO_CONFIG_INT(SvSlashMe, sv_slash_me, 0, 0, 1, CFGFLAG_SERVER, "Whether /me is active on the server or not", 4)

MACRO_CONFIG_INT(ConnTimeout, conn_timeout, 15, 5, 100, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Network timeout", 4)

MACRO_CONFIG_INT(DbgMsg, dbg_msg, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Display or not debug messages", 3)
#endif
