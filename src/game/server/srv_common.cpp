#include <engine/config.h>
#include "srv_common.h"
#include <string.h>

gameobject::gameobject()
: entity(OBJTYPE_GAME)
{
	// select gametype
	if(strcmp(config.gametype, "ctf") == 0)
	{
		gametype = GAMETYPE_CTF;
		dbg_msg("game", "-- Capture The Flag --");
	}
	else if(strcmp(config.gametype, "tdm") == 0)
	{
		gametype = GAMETYPE_TDM;
		dbg_msg("game", "-- Team Death Match --");
	}
	else
	{
		gametype = GAMETYPE_DM;
		dbg_msg("game", "-- Death Match --");
	}
		
	//
	do_warmup(config.warmup);
	game_over_tick = -1;
	sudden_death = 0;
	round_start_tick = server_tick();
	round_count = 0;
	is_teamplay = false;
}

void gameobject::endround()
{
	if(warmup) // game can't end when we are running warmup
		return;
		
	world->paused = true;
	game_over_tick = server_tick();
	sudden_death = 0;
}

void gameobject::resetgame()
{
	world->reset_requested = true;
}

static bool is_separator(char c)
{
	return c == ';' || c == ' ' || c == ',' || c == '\t';
}

void gameobject::startround()
{
	resetgame();
	
	round_start_tick = server_tick();
	sudden_death = 0;
	game_over_tick = -1;
	world->paused = false;
	teamscore[0] = 0;
	teamscore[1] = 0;
	round_count++;
}

void gameobject::cyclemap()
{
	if(!strlen(config.sv_maprotation))
		return;
	// handle maprotation
	char buf[512];
	const char *s = strstr(config.sv_maprotation, config.sv_map);
	if(s == 0)
		s = config.sv_maprotation; // restart rotation
	else
	{
		s += strlen(config.sv_map); // skip this map
		while(is_separator(s[0]))
			s++;
		if(s[0] == 0)
			s = config.sv_maprotation; // restart rotation
	}
		
	int i = 0;
	for(; i < 512; i++)
	{
		buf[i] = s[i];
		if(is_separator(s[i]) || s[i] == 0)
		{
			buf[i] = 0;
			break;
		}
	}
	
	i = 0; // skip spaces
	while(is_separator(buf[i]))
		i++;
	
	dbg_msg("game", "rotating map to %s", &buf[i]);
	strcpy(config.sv_map, &buf[i]);
}

void gameobject::post_reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1)
			players[i].respawn();
	}
}


	
void gameobject::on_player_info_change(class player *p)
{
	const int team_colors[2] = {54090, 10998628};
	if(is_teamplay)
	{
		if(p->team >= 0 || p->team <= 1)
		{
			p->use_custom_color = 1;
			p->color_body = team_colors[p->team];
			p->color_feet = team_colors[p->team];
		}
	}
}


int gameobject::on_player_death(class player *victim, class player *killer, int weapon)
{
	// do scoreing
	if(!killer)
		return 0;
	if(killer == victim)
		victim->score--; // klant arschel
	else
		killer->score++; // good shit
	return 0;
}

void gameobject::do_warmup(int seconds)
{
	warmup = seconds*SERVER_TICK_SPEED;
}


void gameobject::tick()
{
	// do warmup
	if(warmup)
	{
		warmup--;
		if(!warmup)
			resetgame();
	}
	
	if(game_over_tick != -1)
	{
		// game over.. wait for restart
		if(server_tick() > game_over_tick+server_tickspeed()*10)
		{
			cyclemap();
			startround();
		}
	}
}

void gameobject::snap(int snapping_client)
{
	obj_game *game = (obj_game *)snap_new_item(OBJTYPE_GAME, 0, sizeof(obj_game));
	game->paused = world->paused;
	game->game_over = game_over_tick==-1?0:1;
	game->sudden_death = sudden_death;
	
	game->score_limit = config.scorelimit;
	game->time_limit = config.timelimit;
	game->round_start_tick = round_start_tick;
	game->gametype = gametype;
	
	game->warmup = warmup;
	
	game->teamscore[0] = teamscore[0];
	game->teamscore[1] = teamscore[1];
}

int gameobject::getteam(int notthisid)
{
	int numplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1 && players[i].client_id != notthisid)
		{
			numplayers[players[i].team]++;
		}
	}

	return numplayers[0] > numplayers[1] ? 1 : 0;
}

gameobject *gameobj = 0;
