/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H
#undef GAME_VARIABLES_H // this file will be included several times


// client
MACRO_CONFIG_INT(ClPredict, cl_predict, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict client movements")
MACRO_CONFIG_INT(ClNameplates, cl_nameplates, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show name plates")
MACRO_CONFIG_INT(ClNameplatesAlways, cl_nameplates_always, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show name plates disregarding of distance")
MACRO_CONFIG_INT(ClNameplatesTeamcolors, cl_nameplates_teamcolors, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Use team colors for name plates")
MACRO_CONFIG_INT(ClNameplatesSize, cl_nameplates_size, 50, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Size of the name plates from 0 to 100%")
MACRO_CONFIG_INT(ClAutoswitchWeapons, cl_autoswitch_weapons, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup")

MACRO_CONFIG_INT(ClShowhud, cl_showhud, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame HUD")
MACRO_CONFIG_INT(ClShowfps, cl_showfps, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame FPS counter")

MACRO_CONFIG_INT(ClAirjumpindicator, cl_airjumpindicator, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClThreadsoundloading, cl_threadsoundloading, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Load sound files threaded")

MACRO_CONFIG_INT(ClWarningTeambalance, cl_warning_teambalance, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Warn about team balance")

MACRO_CONFIG_INT(ClMouseDeadzone, cl_mouse_deadzone, 300, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMouseFollowfactor, cl_mouse_followfactor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMouseMaxDistance, cl_mouse_max_distance, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(EdShowkeys, ed_showkeys, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

//MACRO_CONFIG_INT(ClFlow, cl_flow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(ClShowWelcome, cl_show_welcome, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMotdTime, cl_motd_time, 10, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long to show the server message of the day")

MACRO_CONFIG_STR(ClVersionServer, cl_version_server, 100, "version.teeworlds.com", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to check for new versions")

MACRO_CONFIG_STR(ClLanguagefile, cl_languagefile, 255, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "What language file to use")
MACRO_CONFIG_STR(ClFontfile, cl_fontfile, 255, "Free Sans Bold.ttf", CFGFLAG_CLIENT|CFGFLAG_SAVE, "What font file to use")

/* other stuff :\ */
MACRO_CONFIG_INT(ClNameplateClientID, cl_nameplate_client_id, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Shows the client id above the player")
MACRO_CONFIG_INT(ClScoreboardClientID, cl_scoreboard_client_id, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Shows the client id in scoreboard")
MACRO_CONFIG_INT(ClShowGhost, cl_show_ghost, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Shows the ghost tee")
MACRO_CONFIG_INT(ClRenderFLow, cl_render_flow, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Renders particle flow (experimental)")

//Hud-Mod
MACRO_CONFIG_INT(ClRenderTime, cl_render_time, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the Server time on or off")
MACRO_CONFIG_INT(ClRenderScoreboard, cl_render_scoreboard, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the game over scoreboard on or off")
MACRO_CONFIG_INT(ClRenderRecordInfo, cl_render_record_info, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the record info on or off when showing the scoreboard")
MACRO_CONFIG_INT(ClRenderWarmup, cl_render_warmup, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes warmup timer on or off")
MACRO_CONFIG_INT(ClRenderBroadcast, cl_render_broadcast, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes broadcast on or off")
MACRO_CONFIG_INT(ClRenderHp, cl_render_hp, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the hp display on or off")
MACRO_CONFIG_INT(ClRenderAmmo, cl_render_ammo, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the ammo display on or off")
MACRO_CONFIG_INT(ClRenderCrosshair, cl_render_crosshair, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the crosshair on or off")
MACRO_CONFIG_INT(ClRenderTeamScore, cl_render_team_score, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the small team score board on or off")
MACRO_CONFIG_INT(ClRenderDmScore, cl_render_dm_score, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes the small DM score board on or off")
MACRO_CONFIG_INT(ClRenderServermsg, cl_render_servermsg, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes in game server messages on or off")
MACRO_CONFIG_INT(ClRenderChat, cl_render_chat, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes in game chat on or off")
MACRO_CONFIG_INT(ClRenderKill, cl_render_kill, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes kill messages on or off")
MACRO_CONFIG_INT(ClRenderVote, cl_render_vote, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes Vote messages on or off")
MACRO_CONFIG_INT(ClRenderViewmode, cl_render_viewmode, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes viewmode display on or off")
MACRO_CONFIG_INT(ClRenderEmotes, cl_render_emotes, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes emoticons on or off")

MACRO_CONFIG_INT(ClClearAll, cl_clear_all, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Cleans the whole screen")

MACRO_CONFIG_INT(ClServermsgsound, cl_servermsgsound, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes servermessage sound on or off")
MACRO_CONFIG_INT(ClChatsound, cl_chatsound, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes chat sound on or off")
MACRO_CONFIG_INT(ClSpreesounds, cl_spreesounds, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turnes spreesounds on or off")

/* Beep-Mod */
MACRO_CONFIG_STR(ClSearchName, cl_search_name, 64, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Names to search for in chat messages")
MACRO_CONFIG_INT(ClAntiSpam, cl_anti_spam, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disables multiple messages")

/* Laser Color */
MACRO_CONFIG_INT(ClLaserColor, cl_laser_color, 11665217, 0, 16777215, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Laser color")

MACRO_CONFIG_INT(PlayerUseCustomColor, player_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors")
MACRO_CONFIG_INT(PlayerColorBody, player_color_body, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player body color")
MACRO_CONFIG_INT(PlayerColorFeet, player_color_feet, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player feet color")
MACRO_CONFIG_STR(PlayerSkin, player_skin, 24, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player skin")

MACRO_CONFIG_INT(UiPage, ui_page, 6, 0, 10, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface page")
MACRO_CONFIG_INT(UiToolboxPage, ui_toolbox_page, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toolbox page")
MACRO_CONFIG_STR(UiServerAddress, ui_server_address, 64, "localhost:8303", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface server address")
MACRO_CONFIG_INT(UiScale, ui_scale, 100, 50, 150, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface scale")
MACRO_CONFIG_INT(UiMousesens, ui_mousesens, 100, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity for menus/editor")

MACRO_CONFIG_INT(UiColorHue, ui_color_hue, 160, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color hue")
MACRO_CONFIG_INT(UiColorSat, ui_color_sat, 70, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color saturation")
MACRO_CONFIG_INT(UiColorLht, ui_color_lht, 175, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color lightness")
MACRO_CONFIG_INT(UiColorAlpha, ui_color_alpha, 228, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface alpha")

MACRO_CONFIG_INT(GfxNoclip, gfx_noclip, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disable clipping")

/* race - client */
MACRO_CONFIG_INT(ClAutoRaceRecord, cl_auto_race_record, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save the best demo of each race")
MACRO_CONFIG_INT(ClDemoName, cl_demo_name, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save the playername within the demo")
MACRO_CONFIG_INT(ClShowOthers, cl_show_others, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Turn other players off in race")
MACRO_CONFIG_INT(ClShowCheckpointDiff, cl_show_checkpoint_diff, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show checkpoint diff")
MACRO_CONFIG_INT(ClShowServerRecord, cl_show_server_record, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show server record")
MACRO_CONFIG_INT(ClRaceGhost, cl_race_ghost, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable ghost")
MACRO_CONFIG_INT(ClRaceSaveGhost, cl_race_save_ghost, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save ghost")
MACRO_CONFIG_INT(ClGhostNamePlates, cl_ghost_nameplates, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ghost nameplates")
MACRO_CONFIG_INT(ClGhostNameplatesAlways, cl_ghost_nameplates_always, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show ghost nameplats disregarding of distance")

MACRO_CONFIG_STR(WaUsername, wa_username, 32, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Teerace username")
MACRO_CONFIG_STR(WaPassword, wa_password, 32, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Teerace password")
MACRO_CONFIG_STR(WaApiToken, wa_api_token, 25, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Api token for webapp")
MACRO_CONFIG_STR(WaWebappIp, wa_webapp_ip, 32, "race.teesites.net", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Webapp IP")
MACRO_CONFIG_STR(WaApiPath, wa_api_path, 32, "/api/1", CFGFLAG_CLIENT|CFGFLAG_SAVE, "initial path to api")

// teecomp
#include "teecomp_vars.h"

// server
MACRO_CONFIG_INT(SvWarmup, sv_warmup, 0, 0, 0, CFGFLAG_SERVER, "Number of seconds to do warmup before round starts")
MACRO_CONFIG_STR(SvMotd, sv_motd, 900, "", CFGFLAG_SERVER, "Message of the day to display for the clients")
MACRO_CONFIG_INT(SvTeamdamage, sv_teamdamage, 0, 0, 1, CFGFLAG_SERVER, "Team damage")
MACRO_CONFIG_STR(SvMaprotation, sv_maprotation, 768, "", CFGFLAG_SERVER, "Maps to rotate between")
MACRO_CONFIG_INT(SvRoundsPerMap, sv_rounds_per_map, 1, 1, 100, CFGFLAG_SERVER, "Number of rounds on each map before rotating")
MACRO_CONFIG_INT(SvPowerups, sv_powerups, 1, 0, 1, CFGFLAG_SERVER, "Allow powerups like ninja")
MACRO_CONFIG_INT(SvScorelimit, sv_scorelimit, 20, 0, 1000, CFGFLAG_SERVER, "Score limit (0 disables)")
MACRO_CONFIG_INT(SvTimelimit, sv_timelimit, 0, 0, 1000, CFGFLAG_SERVER, "Time limit in minutes (0 disables)")
MACRO_CONFIG_STR(SvGametype, sv_gametype, 32, "dm", CFGFLAG_SERVER, "Game type (dm, tdm, ctf)")
MACRO_CONFIG_INT(SvTournamentMode, sv_tournament_mode, 0, 0, 1, CFGFLAG_SERVER, "Tournament mode. When enabled, players joins the server as spectator")
MACRO_CONFIG_INT(SvSpamprotection, sv_spamprotection, 1, 0, 1, CFGFLAG_SERVER, "Spam protection")

MACRO_CONFIG_INT(SvRespawnDelayTDM, sv_respawn_delay_tdm, 3, 0, 10, CFGFLAG_SERVER, "Time needed to respawn after death in tdm gametype")

MACRO_CONFIG_INT(SvSpectatorSlots, sv_spectator_slots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Number of slots to reserve for spectators")
MACRO_CONFIG_INT(SvTeambalanceTime, sv_teambalance_time, 1, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before autobalancing teams")
MACRO_CONFIG_INT(SvInactiveKickTime, sv_inactivekick_time, 3, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before taking care of inactive players")
MACRO_CONFIG_INT(SvInactiveKick, sv_inactivekick, 1, 0, 2, CFGFLAG_SERVER, "How to deal with inactive players (0=move to spectator, 1=move to free spectator slot/kick, 2=kick)")

MACRO_CONFIG_INT(SvLoadMapDefaults, sv_load_map_defaults, 1, 0, 1, CFGFLAG_SERVER, "Load map default settings")

MACRO_CONFIG_INT(SvStrictSpectateMode, sv_strict_spectate_mode, 0, 0, 1, CFGFLAG_SERVER, "Restricts information in spectator mode")
MACRO_CONFIG_INT(SvVoteSpectate, sv_vote_spectate, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to move players to spectators")
MACRO_CONFIG_INT(SvVoteSpectateRejoindelay, sv_vote_spectate_rejoindelay, 3, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before a player can rejoin after being moved to spectators by vote")
MACRO_CONFIG_INT(SvVoteKick, sv_vote_kick, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to kick players")
MACRO_CONFIG_INT(SvVoteKickMin, sv_vote_kick_min, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Minimum number of players required to start a kick vote")
MACRO_CONFIG_INT(SvVoteKickBantime, sv_vote_kick_bantime, 5, 0, 1440, CFGFLAG_SERVER, "The time to ban a player if kicked by vote. 0 makes it just use kick")

// debug
#ifdef CONF_DEBUG // this one can crash the server if not used correctly
	MACRO_CONFIG_INT(DbgDummies, dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "")
#endif

MACRO_CONFIG_INT(DbgFocus, dbg_focus, 0, 0, 1, CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(DbgTuning, dbg_tuning, 0, 0, 1, CFGFLAG_CLIENT, "")
#endif
