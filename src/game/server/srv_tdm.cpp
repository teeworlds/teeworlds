#include <engine/config.h>
#include "srv_common.h"
#include "srv_tdm.h"

void gameobject_tdm::tick()
{
	if(game_over_tick == -1)
	{
		// game is running
		
		// gather some stats
		int totalscore[2] = {0,0};
		int topscore_count = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1)
				totalscore[players[i].team] += players[i].score;
		}
		if (totalscore[0] >= config.scorelimit)
			topscore_count++;
		if (totalscore[1] >= config.scorelimit)
			topscore_count++;
		
		// check score win condition
		if((config.scorelimit > 0 && (totalscore[0] >= config.scorelimit || totalscore[1] >= config.scorelimit)) ||
			(config.timelimit > 0 && (server_tick()-round_start_tick) >= config.timelimit*server_tickspeed()*60))
		{
			if(topscore_count == 1)
				endround();
			else
				sudden_death = 1;
		}
	}
	else
	{
		// game over.. wait for restart
		if(server_tick() > game_over_tick+server_tickspeed()*10)
			startround();
	}
}
