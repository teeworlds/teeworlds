/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
MACRO_CONFIG_INT(key_move_left, 'A', 32, 512)
MACRO_CONFIG_INT(key_move_right, 'D', 32, 512)
MACRO_CONFIG_INT(key_jump, 32, 32, 512)
MACRO_CONFIG_INT(key_fire, 384, 32, 512)
MACRO_CONFIG_INT(key_hook, 385, 32, 512)
MACRO_CONFIG_INT(key_weapon1, '1', 32, 512)
MACRO_CONFIG_INT(key_weapon2, '2', 32, 512)
MACRO_CONFIG_INT(key_weapon3, '3', 32, 512)
MACRO_CONFIG_INT(key_weapon4, '4', 32, 512)
MACRO_CONFIG_INT(key_weapon5, '5', 32, 512)
MACRO_CONFIG_INT(key_weapon6, '6', 32, 512)
MACRO_CONFIG_INT(key_weapon7, '7', 32, 512)

MACRO_CONFIG_INT(key_next_weapon, 382, 32, 512)
MACRO_CONFIG_INT(key_prev_weapon, 383, 32, 512)

MACRO_CONFIG_INT(key_emoticon, 'E', 32, 512)

MACRO_CONFIG_INT(key_chat, 'T', 32, 512)
MACRO_CONFIG_INT(key_teamchat, 'Y', 32, 512)

MACRO_CONFIG_INT(key_console, 256+2, 32, 512)
MACRO_CONFIG_INT(key_remoteconsole, 256+3, 32, 512)

MACRO_CONFIG_INT(key_toggleconsole, 256+4, 32, 512)

MACRO_CONFIG_INT(dbg_bots, 0, 0, 11)

MACRO_CONFIG_INT(cl_predict, 1, 0, 1)
MACRO_CONFIG_INT(cl_nameplates, 0, 0, 1)
MACRO_CONFIG_INT(cl_nameplates_always, 0, 0, 1)
MACRO_CONFIG_INT(cl_dynamic_camera, 1, 0, 1)
MACRO_CONFIG_INT(cl_team, -10, -1, 0)
MACRO_CONFIG_INT(cl_autoswitch_weapons, 0, 0, 1)
MACRO_CONFIG_INT(cl_show_player_ids, 0, 0, 1)

MACRO_CONFIG_INT(cl_show_welcome, 1, 0, 1)

MACRO_CONFIG_INT(player_use_custom_color, 0, 0, 1)
MACRO_CONFIG_INT(player_color_body, 65408, 0, 0)
MACRO_CONFIG_INT(player_color_feet, 65408, 0, 0)
MACRO_CONFIG_STR(player_skin, 64, "default")

MACRO_CONFIG_INT(dbg_firedelay, 0, 0, 1)

MACRO_CONFIG_INT(ui_page, 1, 0, 5)
MACRO_CONFIG_STR(ui_server_address, 128, "localhost:8303")
MACRO_CONFIG_INT(ui_scale, 100, 1, 100000)

MACRO_CONFIG_INT(ui_color_hue, 160, 0, 255)
MACRO_CONFIG_INT(ui_color_sat, 70, 0, 255)
MACRO_CONFIG_INT(ui_color_lht, 175, 0, 255)
MACRO_CONFIG_INT(ui_color_alpha, 228, 0, 255)


MACRO_CONFIG_INT(sv_warmup, 0, 0, 0)
MACRO_CONFIG_STR(sv_msg, 512, "")
MACRO_CONFIG_INT(sv_teamdamage, 0, 0, 1)
MACRO_CONFIG_STR(sv_maprotation, 512, "")
MACRO_CONFIG_INT(sv_powerups, 1, 0, 1)
MACRO_CONFIG_INT(sv_scorelimit, 20, 0, 1000)
MACRO_CONFIG_INT(sv_timelimit, 0, 0, 1000)
MACRO_CONFIG_STR(sv_gametype, 32, "dm")
MACRO_CONFIG_INT(sv_restart, 0, 0, 120)
MACRO_CONFIG_INT(sv_kick, -1, 0, 0)
MACRO_CONFIG_INT(sv_tournament_mode, 0, 0, 1)



