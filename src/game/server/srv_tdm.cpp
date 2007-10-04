#include <engine/config.h>
#include "srv_common.h"
#include "srv_tdm.h"

void gameobject_tdm::tick()
{
	if(game_over_tick == -1)
	{
		// game is running
		teamscore[0] = 0;
		teamscore[1] = 0;
		
		// gather some stats
		int topscore_count = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1)
				teamscore[players[i].team] += players[i].score;
		}
		if (teamscore[0] >= config.scorelimit)
			topscore_count++;
		if (teamscore[1] >= config.scorelimit)
			topscore_count++;
		
		// check score win condition
		if((config.scorelimit > 0 && (teamscore[0] >= config.scorelimit || teamscore[1] >= config.scorelimit)) ||
			(config.timelimit > 0 && (server_tick()-round_start_tick) >= config.timelimit*server_tickspeed()*60))
		{
			if(topscore_count == 1)
				endround();
			else
				sudden_death = 1;
		}
	}
	
	gameobject::tick();
}
