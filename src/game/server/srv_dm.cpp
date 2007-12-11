/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/config.h>
#include "srv_common.h"
#include "srv_dm.h"

void gameobject_dm::tick()
{
	if(game_over_tick == -1)
	{
		// game is running
		
		// gather some stats
		int topscore = 0;
		int topscore_count = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(players[i].client_id != -1)
			{
				if(players[i].score > topscore)
				{
					topscore = players[i].score;
					topscore_count = 1;
				}
				else if(players[i].score == topscore)
					topscore_count++;
			}
		}
		
		// check score win condition
		if((config.sv_scorelimit > 0 && topscore >= config.sv_scorelimit) ||
			(config.sv_timelimit > 0 && (server_tick()-round_start_tick) >= config.sv_timelimit*server_tickspeed()*60))
		{
			if(topscore_count == 1)
				endround();
			else
				sudden_death = 1;
		}
	}
	
	gameobject::tick();
}

