/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "dm.hpp"

void GAMECONTROLLER_DM::tick()
{
	do_player_score_wincheck();
	GAMECONTROLLER::tick();
}
