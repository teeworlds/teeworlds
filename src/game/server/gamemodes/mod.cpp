/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "mod.hpp"

GAMECONTROLLER_MOD::GAMECONTROLLER_MOD()
{
	gametype = "MOD";
	//game_flags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void GAMECONTROLLER_MOD::tick()
{
	// this is the main part of the gamemode, this function is run every tick
	do_player_score_wincheck(); // checks for winners, no teams version
	//do_team_score_wincheck(); // checks for winners, two teams version
	
	GAMECONTROLLER::tick();
}
