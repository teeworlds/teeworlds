MACRO_CONFIG_INT(TcNameplateShadow, tc_nameplate_shadow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable name plate shadow")
MACRO_CONFIG_INT(TcNameplateScore, tc_nameplate_score, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Display score on name plates")
MACRO_CONFIG_INT(TcColoredNameplates, tc_colored_nameplates, 0, 0, 3, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable colored name plates")
MACRO_CONFIG_INT(TcColoredNameplatesTeam1, tc_colored_nameplates_team1, 16739179, 0, 16777215, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Red team/team mates name plate color")
MACRO_CONFIG_INT(TcColoredNameplatesTeam2, tc_colored_nameplates_team2, 7053311, 0, 16777215, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Blue team/enemies name plate color")

MACRO_CONFIG_INT(TcColoredTeesMethod, tc_colored_tees_method, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable enemy based skin colors")
MACRO_CONFIG_INT(TcDmColorsTeam1, tc_dm_colors_team1, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Use DM colors for red team/team mates")
MACRO_CONFIG_INT(TcDmColorsTeam2, tc_dm_colors_team2, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Use DM colors for blue team/enemies")
MACRO_CONFIG_INT(TcColoredTeesTeam1, tc_colored_tees_team1, 16739179, 0, 16777215, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Red team/team mates color")
MACRO_CONFIG_INT(TcColoredTeesTeam2, tc_colored_tees_team2, 7053311, 0, 16777215, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Blue team/enemies color")

MACRO_CONFIG_INT(TcForcedSkinsMethod, tc_forced_skins_method, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable enemy based forced skins")
MACRO_CONFIG_INT(TcForceSkinTeam1, tc_force_skin_team1, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Force a skin for red team/your team/DM matchs")
MACRO_CONFIG_INT(TcForceSkinTeam2, tc_force_skin_team2, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Force a skin for blue team/opponents")
MACRO_CONFIG_STR(TcForcedSkin1, tc_forced_skin1, 64, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Forced skin for red/mates/DM matchs")
MACRO_CONFIG_STR(TcForcedSkin2, tc_forced_skin2, 64, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Forced skin for blue/opponents")

MACRO_CONFIG_INT(TcHudMatch, tc_hud_match, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Make HUD match tees' colors")
MACRO_CONFIG_INT(TcSpeedmeter, tc_speedmeter, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Display speed meter")
MACRO_CONFIG_INT(TcSpeedmeterAccel, tc_speedmeter_accel, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Speed meter shows acceleration")

MACRO_CONFIG_INT(TcAutoscreen, tc_autoscreen, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable auto screenshot")
MACRO_CONFIG_INT(TcAutoStatscreen, tc_auto_statscreen, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable auto screenshot for stats")

MACRO_CONFIG_INT(TcColoredFlags, tc_colored_flags, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Make flags colors match tees colors")
MACRO_CONFIG_INT(TcHideCarrying, tc_hide_carrying, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Hide the flag if you're carrying it")

MACRO_CONFIG_INT(TcStatboardInfos, tc_statboard_infos, 495, 1, 1023, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Mask of infos to display on the global statboard")
MACRO_CONFIG_INT(TcStatId, tc_stat_id, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show player id in statboards")
