/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_config.h>
#include "gs_common.h"
#include "gs_game_tdm.h"

gameobject_tdm::gameobject_tdm()
{
	is_teamplay = true;
}

int gameobject_tdm::on_player_death(class player *victim, class player *killer, int weapon)
{
	gameobject::on_player_death(victim, killer, weapon);
	
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

void gameobject_tdm::tick()
{
	do_team_score_wincheck();
	gameobject::tick();
}
