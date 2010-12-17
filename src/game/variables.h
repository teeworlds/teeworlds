/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H
#undef GAME_VARIABLES_H // this file will be included several times


// client
MACRO_CONFIG_INT(ClPredict, cl_predict, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict client movements", -1)
MACRO_CONFIG_INT(ClNameplates, cl_nameplates, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show nameplates", -1)
MACRO_CONFIG_INT(ClNameplatesAlways, cl_nameplates_always, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show nameplats disregarding of distance", -1)
MACRO_CONFIG_INT(ClAutoswitchWeapons, cl_autoswitch_weapons, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup", -1)

MACRO_CONFIG_INT(ClShowfps, cl_showfps, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame FPS counter", -1)

MACRO_CONFIG_INT(ClAirjumpindicator, cl_airjumpindicator, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)
MACRO_CONFIG_INT(ClThreadsoundloading, cl_threadsoundloading, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)

MACRO_CONFIG_INT(ClWarningTeambalance, cl_warning_teambalance, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Warn about team balance", -1)

MACRO_CONFIG_INT(ClMouseDeadzone, cl_mouse_deadzone, 300, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)
MACRO_CONFIG_INT(ClMouseFollowfactor, cl_mouse_followfactor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)
MACRO_CONFIG_INT(ClMouseMaxDistance, cl_mouse_max_distance, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)

MACRO_CONFIG_INT(ClDDRaceCheats, cl_race_cheats, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "",0)

MACRO_CONFIG_INT(EdShowkeys, ed_showkeys, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)

MACRO_CONFIG_INT(ClFlow, cl_flow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)

MACRO_CONFIG_INT(ClShowWelcome, cl_show_welcome, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "", -1)
MACRO_CONFIG_INT(ClMotdTime, cl_motd_time, 10, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long to show the server message of the day", -1)

MACRO_CONFIG_STR(ClVersionServer, cl_version_server, 100, "version.teeworlds.com", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to check for new versions", -1)

MACRO_CONFIG_STR(ClLanguagefile, cl_languagefile, 255, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "What language file to use", -1)

MACRO_CONFIG_INT(PlayerUseCustomColor, player_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors", -1)
MACRO_CONFIG_INT(PlayerColorBody, player_color_body, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player body color", -1)
MACRO_CONFIG_INT(PlayerColorFeet, player_color_feet, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player feet color", -1)
MACRO_CONFIG_STR(PlayerSkin, player_skin, 24, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player skin", -1)

MACRO_CONFIG_INT(UiPage, ui_page, 5, 0, 9, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface page", -1)
MACRO_CONFIG_INT(UiToolboxPage, ui_toolbox_page, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toolbox page", -1)
MACRO_CONFIG_STR(UiServerAddress, ui_server_address, 25, "localhost:8303", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface server address", -1)
MACRO_CONFIG_INT(UiScale, ui_scale, 100, 1, 100000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface scale", -1)

MACRO_CONFIG_INT(UiColorHue, ui_color_hue, 160, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color hue", -1)
MACRO_CONFIG_INT(UiColorSat, ui_color_sat, 70, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color saturation", -1)
MACRO_CONFIG_INT(UiColorLht, ui_color_lht, 175, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color lightness", -1)
MACRO_CONFIG_INT(UiColorAlpha, ui_color_alpha, 228, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface alpha", -1)

MACRO_CONFIG_INT(GfxNoclip, gfx_noclip, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disable clipping", -1)

// server
MACRO_CONFIG_INT(SvWarmup, sv_warmup, 0, 0, 0, CFGFLAG_SERVER, "Number of seconds to do warmup before round starts", 3)
MACRO_CONFIG_STR(SvMotd, sv_motd, 900, "", CFGFLAG_SERVER, "Message of the day to display for the clients", 3)
/*MACRO_CONFIG_INT(SvTeamdamage, sv_teamdamage, 0, 0, 1, CFGFLAG_SERVER, "Team damage")
MACRO_CONFIG_STR(SvMaprotation, sv_maprotation, 768, "", CFGFLAG_SERVER, "Maps to rotate between")
MACRO_CONFIG_INT(SvRoundsPerMap, sv_rounds_per_map, 1, 1, 100, CFGFLAG_SERVER, "Number of rounds on each map before rotating")
MACRO_CONFIG_INT(SvPowerups, sv_powerups, 1, 0, 1, CFGFLAG_SERVER, "Allow powerups like ninja")
MACRO_CONFIG_INT(SvScorelimit, sv_scorelimit, 20, 0, 1000, CFGFLAG_SERVER, "Score limit (0 disables)")
MACRO_CONFIG_INT(SvTimelimit, sv_timelimit, 0, 0, 1000, CFGFLAG_SERVER, "Time limit in minutes (0 disables)")
MACRO_CONFIG_STR(SvGametype, sv_gametype, 32, "dm", CFGFLAG_SERVER, "Game type (dm, tdm, ctf)")*/
MACRO_CONFIG_INT(SvTournamentMode, sv_tournament_mode, 0, 0, 1, CFGFLAG_SERVER, "Tournament mode. When enabled, players joins the server as spectator", 3)
MACRO_CONFIG_INT(SvSpamprotection, sv_spamprotection, 1, 0, 1, CFGFLAG_SERVER, "Spam protection",3)

MACRO_CONFIG_INT(SvSpectatorSlots, sv_spectator_slots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Number of slots to reserve for spectators", 3)
//MACRO_CONFIG_INT(SvTeambalanceTime, sv_teambalance_time, 1, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before autobalancing teams")
MACRO_CONFIG_INT(SvInactiveKickTime, sv_inactivekick_time, 180, 0, 9999, CFGFLAG_SERVER, "How many minutes to wait before taking care of inactive players", 3)
MACRO_CONFIG_INT(SvInactiveKick, sv_inactivekick, 1, 0, 2, CFGFLAG_SERVER, "How to deal with inactive players (0=move to spectator, 1=move to free spectator slot/kick, 2=kick)", 3)

MACRO_CONFIG_INT(SvVoteKick, sv_vote_kick, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to kick players", 3)
MACRO_CONFIG_INT(SvVoteKickBantime, sv_vote_kick_bantime, 5, 0, 1440, CFGFLAG_SERVER, "The time to ban a player if kicked by vote. 0 makes it just use kick", 3)

// debug
#ifdef CONF_DEBUG // this one can crash the server if not used correctly
	MACRO_CONFIG_INT(DbgDummies, dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "", 3)
#endif

MACRO_CONFIG_INT(DbgFocus, dbg_focus, 0, 0, 1, CFGFLAG_CLIENT, "", -1)
MACRO_CONFIG_INT(DbgTuning, dbg_tuning, 0, 0, 1, CFGFLAG_CLIENT, "", -1)
#endif
