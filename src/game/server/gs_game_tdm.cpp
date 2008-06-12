/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_config.h>
#include "gs_common.hpp"
#include "gs_game_tdm.hpp"

GAMECONTROLLER_TDM::GAMECONTROLLER_TDM()
{
	is_teamplay = true;
}

int GAMECONTROLLER_TDM::on_player_death(class PLAYER *victim, class PLAYER *killer, int weapon)
{
	GAMECONTROLLER::on_player_death(victim, killer, weapon);
	
	if(weapon >= 0)
	{
		// do team scoring
		if(killer == victim)
			teamscore[killer->team&1]--; // klant arschel
		else
			teamscore[killer->team&1]++; // good shit
	}
	return 0;
}

void GAMECONTROLLER_TDM::tick()
{
	do_team_score_wincheck();
	GAMECONTROLLER::tick();
}
