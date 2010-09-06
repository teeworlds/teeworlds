#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H
#undef GAME_VARIABLES_H // this file will be included several times


// client
MACRO_CONFIG_INT(ClPredict, cl_predict, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict client movements",0)
MACRO_CONFIG_INT(ClNameplates, cl_nameplates, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show nameplates",0)
MACRO_CONFIG_INT(ClNameplatesAlways, cl_nameplates_always, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show nameplats disregarding of distance",0)
MACRO_CONFIG_INT(ClAutoswitchWeapons, cl_autoswitch_weapons, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup",0)

MACRO_CONFIG_INT(ClShowfps, cl_showfps, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame FPS counter",0)

MACRO_CONFIG_INT(ClAirjumpindicator, cl_airjumpindicator, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClThreadsoundloading, cl_threadsoundloading, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(ClWarningTeambalance, cl_warning_teambalance, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Warn about team balance",0)

MACRO_CONFIG_INT(ClMouseDeadzone, cl_mouse_deadzone, 300, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClMouseFollowfactor, cl_mouse_followfactor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClMouseMaxDistance, cl_mouse_max_distance, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(ClRaceCheats, cl_race_cheats, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(EdShowkeys, ed_showkeys, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(ClFlow, cl_flow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(ClShowWelcome, cl_show_welcome, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClMotdTime, cl_motd_time, 10, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long to show the server message of the day",0)

MACRO_CONFIG_STR(ClVersionServer, cl_version_server, 100, "version.teeworlds.com", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to check for new versions",0)

MACRO_CONFIG_STR(ClLanguagefile, cl_languagefile, 255, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "What language file to use",0)

MACRO_CONFIG_INT(PlayerUseCustomColor, player_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors",0)
MACRO_CONFIG_INT(PlayerColorBody, player_color_body, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player body color",0)
MACRO_CONFIG_INT(PlayerColorFeet, player_color_feet, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player feet color",0)
MACRO_CONFIG_STR(PlayerSkin, player_skin, 64, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player skin",0)

MACRO_CONFIG_INT(UiPage, ui_page, 5, 0, 9, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface page",0)
MACRO_CONFIG_INT(UiToolboxPage, ui_toolbox_page, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toolbox page",0)
MACRO_CONFIG_STR(UiServerAddress, ui_server_address, 25, "localhost:8303", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface server address",0)
MACRO_CONFIG_INT(UiScale, ui_scale, 100, 1, 100000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface scale",0)

MACRO_CONFIG_INT(UiColorHue, ui_color_hue, 160, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color hue",0)
MACRO_CONFIG_INT(UiColorSat, ui_color_sat, 70, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color saturation",0)
MACRO_CONFIG_INT(UiColorLht, ui_color_lht, 175, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color lightness",0)
MACRO_CONFIG_INT(UiColorAlpha, ui_color_alpha, 228, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface alpha",0)

MACRO_CONFIG_INT(GfxNoclip, gfx_noclip, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disable clipping",0)
MACRO_CONFIG_INT(GfxClearFull, gfx_full_clear, 0, 0, 1, CFGFLAG_CLIENT, "Only show the game tile",0)

/* race - client */
MACRO_CONFIG_INT(ClAutoRecord, cl_auto_record, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save the best demo of each race",0)
MACRO_CONFIG_INT(ClShowOthers, cl_show_others, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turn other players off in race",0)
MACRO_CONFIG_INT(ClRenderSpeedmeter, cl_render_speedmeter, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Render in game speedmeter",0)
MACRO_CONFIG_INT(ClSpeedmeterAccel, cl_speedmeter_accel, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show acceleration",0)
MACRO_CONFIG_INT(ClShowCheckpointDiff, cl_show_checkpoint_diff, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show checkpoint diff",0)
MACRO_CONFIG_INT(ClShowRecords, cl_show_records, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show records",0)
MACRO_CONFIG_INT(ClShowServerRecord, cl_show_server_record, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show server record",0)
MACRO_CONFIG_INT(ClShowLocalRecord, cl_show_local_record, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show personal best",0)

/* DownloadExtension | Mod by KillaBilla */
MACRO_CONFIG_INT(ClDownloadExtensionSize, cl_downloadextension_size, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionSpeed, cl_downloadextension_speed, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionTimeElapsed, cl_downloadextension_timeelapsed, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionTimeRemaining, cl_downloadextension_timeremaining, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionStatusPercent, cl_downloadextension_statuspercent, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionStatusBar, cl_downloadextension_statusbar, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionStatusBar_r, cl_downloadextension_statusbar_r, 255, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionStatusBar_g, cl_downloadextension_statusbar_g, 255, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionStatusBar_b, cl_downloadextension_statusbar_b, 255, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)
MACRO_CONFIG_INT(ClDownloadExtensionStatusBar_a, cl_downloadextension_statusbar_a, 191, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(ClArrows, cl_arrows, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show arrows in place of text for player streams",0)
MACRO_CONFIG_INT(ClTextSize, cl_text_size, 100, 20, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Text size (%)",0)

// server

/*MACRO_CONFIG_INT(SvTeamdamage, sv_teamdamage, 0, 0, 1, CFGFLAG_SERVER, "Team damage",0)
MACRO_CONFIG_STR(SvMaprotation, sv_maprotation, 768, "", CFGFLAG_SERVER, "Maps to rotate between",0)
MACRO_CONFIG_INT(SvRoundsPerMap, sv_rounds_per_map, 1, 1, 100, CFGFLAG_SERVER, "Number of rounds on each map before rotating",0)
MACRO_CONFIG_INT(SvPowerups, sv_powerups, 1, 0, 1, CFGFLAG_SERVER, "Allow powerups like ninja",0)
MACRO_CONFIG_INT(SvScorelimit, sv_scorelimit, 20, 0, 1000, CFGFLAG_SERVER, "Score limit (0 disables)",0)
MACRO_CONFIG_INT(SvTimelimit, sv_timelimit, 0, 0, 1000, CFGFLAG_SERVER, "Time limit in minutes (0 disables)",0)
MACRO_CONFIG_STR(SvGametype, sv_gametype, 32, "dm", CFGFLAG_SERVER, "Game type (dm, tdm, ctf)",0)*/
//MACRO_CONFIG_INT(SvTeambalanceTime, sv_teambalance_time, 1, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before autobalancing teams",0)
//MACRO_CONFIG_INT(SvVoteScorelimit, sv_vote_scorelimit, 0, 0, 1, CFGFLAG_SERVER, "Allow voting to change score limit",0)
//MACRO_CONFIG_INT(SvVoteTimelimit, sv_vote_timelimit, 0, 0, 1, CFGFLAG_SERVER, "Allow voting to change time limit",0)
// debug
#ifdef CONF_DEBUG // this one can crash the server if not used correctly
	MACRO_CONFIG_INT(DbgDummies, dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "",0)
#endif

MACRO_CONFIG_INT(DbgFocus, dbg_focus, 0, 0, 1, CFGFLAG_CLIENT, "",0)
MACRO_CONFIG_INT(DbgTuning, dbg_tuning, 0, 0, 1, CFGFLAG_CLIENT, "",0)
#endif
