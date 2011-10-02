MACRO_CONFIG_INT(TcNameplateScore, tc_nameplate_score, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Display score on name plates")

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

MACRO_CONFIG_INT(TcColoredFlags, tc_colored_flags, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Make flags colors match tees colors")
MACRO_CONFIG_INT(TcHideCarrying, tc_hide_carrying, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Hide the flag if you're carrying it")

MACRO_CONFIG_INT(TcStatboardInfos, tc_statboard_infos, 1335, 1, 2047, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Mask of infos to display on the global statboard")
MACRO_CONFIG_INT(TcScoreboardInfos, tc_scoreboard_infos, 15, 1, 15, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Mask of infos to display on the scoreboard")
MACRO_CONFIG_INT(TcStatId, tc_stat_id, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show player id in statboards")

MACRO_CONFIG_INT(TcStatScreenshot, tc_stat_screenshot, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Automatically take game over statboard screenshot")
MACRO_CONFIG_INT(TcStatScreenshotMax, tc_stat_screenshot_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically created statboard screenshots (0 = no limit)")
