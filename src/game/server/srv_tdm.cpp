/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/config.h>
#include "srv_common.h"
#include "srv_tdm.h"

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
	if(game_over_tick == -1 && !warmup)
	{
		// check score win condition
		if((config.scorelimit > 0 && (teamscore[0] >= config.scorelimit || teamscore[1] >= config.scorelimit)) ||
			(config.timelimit > 0 && (server_tick()-round_start_tick) >= config.timelimit*server_tickspeed()*60))
		{
			if(teamscore[0] != teamscore[0])
				endround();
			else
				sudden_death = 1;
		}
	}
	
	gameobject::tick();
}
