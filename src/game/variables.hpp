/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
MACRO_CONFIG_INT(cl_predict, 1, 0, 1)
MACRO_CONFIG_INT(cl_nameplates, 0, 0, 1)
MACRO_CONFIG_INT(cl_nameplates_always, 0, 0, 1)
MACRO_CONFIG_INT(cl_autoswitch_weapons, 0, 0, 1)

MACRO_CONFIG_INT(cl_showfps, 0, 0, 1)

MACRO_CONFIG_INT(cl_airjumpindicator, 1, 0, 1)
MACRO_CONFIG_INT(cl_threadsoundloading, 0, 0, 1)


MACRO_CONFIG_INT(cl_warning_tuning, 1, 0, 1)
MACRO_CONFIG_INT(cl_warning_teambalance, 1, 0, 1)

MACRO_CONFIG_INT(cl_mouse_deadzone, 300, 0, 0)
MACRO_CONFIG_INT(cl_mouse_followfactor, 60, 0, 200)
MACRO_CONFIG_INT(cl_mouse_max_distance, 800, 0, 0)

MACRO_CONFIG_INT(cl_layershot, 0, 0, 1)

MACRO_CONFIG_INT(ed_showkeys, 0, 0, 1)

MACRO_CONFIG_INT(cl_flow, 0, 0, 1)

MACRO_CONFIG_INT(cl_show_welcome, 1, 0, 1)
MACRO_CONFIG_INT(cl_motd_time, 10, 0, 100)

MACRO_CONFIG_STR(cl_version_server, 100, "version.teeworlds.com")

MACRO_CONFIG_INT(player_use_custom_color, 0, 0, 1)
MACRO_CONFIG_INT(player_color_body, 65408, 0, 0)
MACRO_CONFIG_INT(player_color_feet, 65408, 0, 0)
MACRO_CONFIG_STR(player_skin, 64, "default")

MACRO_CONFIG_INT(dbg_dummies, 0, 0, 11)
MACRO_CONFIG_INT(dbg_firedelay, 0, 0, 1)
MACRO_CONFIG_INT(dbg_flow, 0, 0, 1)
MACRO_CONFIG_INT(dbg_tuning, 0, 0, 1)

MACRO_CONFIG_INT(ui_page, 5, 0, 9)
MACRO_CONFIG_STR(ui_server_address, 128, "localhost:8303")
MACRO_CONFIG_INT(ui_scale, 100, 1, 100000)

MACRO_CONFIG_INT(ui_color_hue, 160, 0, 255)
MACRO_CONFIG_INT(ui_color_sat, 70, 0, 255)
MACRO_CONFIG_INT(ui_color_lht, 175, 0, 255)
MACRO_CONFIG_INT(ui_color_alpha, 228, 0, 255)


MACRO_CONFIG_INT(sv_warmup, 0, 0, 0)
MACRO_CONFIG_STR(sv_motd, 900, "")
MACRO_CONFIG_INT(sv_teamdamage, 0, 0, 1)
MACRO_CONFIG_STR(sv_maprotation, 768, "")
MACRO_CONFIG_STR(sv_maplist, 768, "")
MACRO_CONFIG_INT(sv_rounds_per_map, 1, 1, 100)
MACRO_CONFIG_INT(sv_powerups, 1, 0, 1)
MACRO_CONFIG_INT(sv_scorelimit, 20, 0, 1000)
MACRO_CONFIG_INT(sv_timelimit, 0, 0, 1000)
MACRO_CONFIG_STR(sv_gametype, 32, "dm")
MACRO_CONFIG_INT(sv_tournament_mode, 0, 0, 1)
MACRO_CONFIG_INT(sv_spamprotection, 1, 0, 1)

MACRO_CONFIG_INT(sv_spectator_slots, 0, 0, 12)
MACRO_CONFIG_INT(sv_teambalance_time, 1, 0, 1000)

MACRO_CONFIG_INT(sv_vote_map, 1, 0, 1)
MACRO_CONFIG_INT(sv_vote_kick, 1, 0, 1)
MACRO_CONFIG_INT(sv_vote_scorelimit, 0, 0, 1)
MACRO_CONFIG_INT(sv_vote_timelimit, 0, 0, 1)
