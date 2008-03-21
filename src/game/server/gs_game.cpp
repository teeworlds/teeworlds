/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
#include <game/g_mapitems.h>
#include "gs_common.h"

gameobject::gameobject()
: entity(NETOBJTYPE_GAME)
{
	// select gametype
	if(strcmp(config.sv_gametype, "ctf") == 0)
	{
		gametype = GAMETYPE_CTF;
		dbg_msg("game", "-- Capture The Flag --");
	}
	else if(strcmp(config.sv_gametype, "tdm") == 0)
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
	do_warmup(config.sv_warmup);
	game_over_tick = -1;
	sudden_death = 0;
	round_start_tick = server_tick();
	round_count = 0;
	is_teamplay = false;
	teamscore[0] = 0;
	teamscore[1] = 0;	
}

// UGLY!!!!
extern vec2 spawn_points[3][64];
extern int num_spawn_points[3];

bool gameobject::on_entity(int index, vec2 pos)
{
	int type = -1;
	int subtype = 0;
	
	if(index == ENTITY_SPAWN)
		spawn_points[0][num_spawn_points[0]++] = pos;
	else if(index == ENTITY_SPAWN_RED)
		spawn_points[1][num_spawn_points[1]++] = pos;
	else if(index == ENTITY_SPAWN_BLUE)
		spawn_points[2][num_spawn_points[2]++] = pos;
	else if(index == ENTITY_ARMOR_1)
		type = POWERUP_ARMOR;
	else if(index == ENTITY_HEALTH_1)
		type = POWERUP_HEALTH;
	else if(index == ENTITY_WEAPON_SHOTGUN)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_SHOTGUN;
	}
	else if(index == ENTITY_WEAPON_GRENADE)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_GRENADE;
	}
	else if(index == ENTITY_WEAPON_RIFLE)
	{
		type = POWERUP_WEAPON;
		subtype = WEAPON_RIFLE;
	}
	else if(index == ENTITY_POWERUP_NINJA)
	{
		type = POWERUP_NINJA;
		subtype = WEAPON_NINJA;
	}
	
	if(type != -1)
	{
		powerup *ppower = new powerup(type, subtype);
		ppower->pos = pos;
		return true;
	}

	return false;
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

static bool is_separator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

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
	const char *map_rotation = config.sv_maprotation;
	const char *current_map = config.sv_map;
	
	int current_map_len = strlen(current_map);
	const char *next_map = map_rotation;
	while(*next_map)
	{
		int wordlen = 0;
		while(next_map[wordlen] && !is_separator(next_map[wordlen]))
			wordlen++;
		
		if(wordlen == current_map_len && strncmp(next_map, current_map, current_map_len) == 0)
		{
			// map found
			next_map += current_map_len;
			while(*next_map && is_separator(*next_map))
				next_map++;
				
			break;
		}
		
		next_map++;
	}
	
	// restart rotation
	if(next_map[0] == 0)
		next_map = map_rotation;

	// cut out the next map	
	char buf[512];
	for(int i = 0; i < 512; i++)
	{
		buf[i] = next_map[i];
		if(is_separator(next_map[i]) || next_map[i] == 0)
		{
			buf[i] = 0;
			break;
		}
	}
	
	// skip spaces
	int i = 0;
	while(is_separator(buf[i]))
		i++;
	
	dbg_msg("game", "rotating map to %s", &buf[i]);
	str_copy(config.sv_map, &buf[i], sizeof(config.sv_map));
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
	const int team_colors[2] = {65387, 10223467};
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
		victim->score--; // suicide
	else
	{
		if(is_teamplay && victim->team == killer->team)
			killer->score--; // teamkill
		else
			killer->score++; // normal kill
	}
	return 0;
}

void gameobject::do_warmup(int seconds)
{
	warmup = seconds*server_tickspeed();
}

bool gameobject::is_friendly_fire(int cid1, int cid2)
{
	if(cid1 == cid2)
		return false;
	
	if(is_teamplay)
	{
		if(players[cid1].team == players[cid2].team)
			return true;
	}
	
	return false;
}

void gameobject::tick()
{
	// do warmup
	if(warmup)
	{
		warmup--;
		if(!warmup)
			startround();
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
	
	
	// update browse info
	int prog = -1;
	if(config.sv_timelimit > 0)
		prog = max(prog, (server_tick()-round_start_tick) * 100 / (config.sv_timelimit*server_tickspeed()*60));

	if(config.sv_scorelimit)
	{
		if(is_teamplay)
		{
			prog = max(prog, (teamscore[0]*100)/config.sv_scorelimit);
			prog = max(prog, (teamscore[1]*100)/config.sv_scorelimit);
		}
		else
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(players[i].client_id != -1)
					prog = max(prog, (players[i].score*100)/config.sv_scorelimit);
			}
		}
	}

	if(warmup)
		prog = -1;
		
	server_setbrowseinfo(gametype, prog);
}

void gameobject::snap(int snapping_client)
{
	NETOBJ_GAME *game = (NETOBJ_GAME *)snap_new_item(NETOBJTYPE_GAME, 0, sizeof(NETOBJ_GAME));
	game->paused = world->paused;
	game->game_over = game_over_tick==-1?0:1;
	game->sudden_death = sudden_death;
	
	game->score_limit = config.sv_scorelimit;
	game->time_limit = config.sv_timelimit;
	game->round_start_tick = round_start_tick;
	game->gametype = gametype;
	
	game->warmup = warmup;
	
	game->teamscore_red = teamscore[0];
	game->teamscore_blue = teamscore[1];
}

int gameobject::getteam(int notthisid)
{
	int numplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1 && players[i].client_id != notthisid)
		{
			if(players[i].team == 0 || players[i].team == 1)
				numplayers[players[i].team]++;
		}
	}

	return numplayers[0] > numplayers[1] ? 1 : 0;
}

void gameobject::do_player_score_wincheck()
{
	if(game_over_tick == -1  && !warmup)
	{
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
}

void gameobject::do_team_score_wincheck()
{
	if(game_over_tick == -1 && !warmup)
	{
		// check score win condition
		if((config.sv_scorelimit > 0 && (teamscore[0] >= config.sv_scorelimit || teamscore[1] >= config.sv_scorelimit)) ||
			(config.sv_timelimit > 0 && (server_tick()-round_start_tick) >= config.sv_timelimit*server_tickspeed()*60))
		{
			if(teamscore[0] != teamscore[1])
				endround();
			else
				sudden_death = 1;
		}
	}
}

int gameobject::clampteam(int team)
{
	if(team < 0) // spectator
		return -1;
	if(is_teamplay)
		return team&1;
	return  0;
}

gameobject *gameobj = 0;
