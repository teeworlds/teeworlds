#include "../game/game_variables.h"

MACRO_CONFIG_INT(screen_width, 800, 0, 0)
MACRO_CONFIG_INT(screen_height, 600, 0, 0)
MACRO_CONFIG_INT(fullscreen, 1, 0, 1)
MACRO_CONFIG_INT(color_depth, 24, 16, 24)
MACRO_CONFIG_INT(vsync, 1, 0, 1)
MACRO_CONFIG_INT(debug, 0, 0, 1)
MACRO_CONFIG_INT(display_all_modes, 0, 0, 1)
MACRO_CONFIG_INT(volume, 200, 0, 255)
MACRO_CONFIG_INT(cpu_throttle, 0, 0, 1)
MACRO_CONFIG_STR(player_name, 32, "nameless tee")
MACRO_CONFIG_STR(clan_name, 32, "")
MACRO_CONFIG_STR(password, 32, "")


MACRO_CONFIG_STR(masterserver, 128, "master.teewars.com")

MACRO_CONFIG_INT(sv_port, 8303, 0, 0)
