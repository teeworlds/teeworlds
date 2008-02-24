/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_server_interface.h>
#include <game/g_mapitems.h>
#include "gs_common.h"
#include "gs_game_ctf.h"

gameobject_ctf::gameobject_ctf()
{
	is_teamplay = true;
}

bool gameobject_ctf::on_entity(int index, vec2 pos)
{
	if(gameobject::on_entity(index, pos))
		return true;
	
	int team = -1;
	if(index == ENTITY_FLAGSTAND_RED) team = 0;
	if(index == ENTITY_FLAGSTAND_BLUE) team = 1;
	if(team == -1)
		return false;
		
	flag *f = new flag(team);
	f->stand_pos = pos;
	f->pos = pos;
	flags[team] = f;
	return true;
}

void gameobject_ctf::on_player_spawn(class player *p)
{
}

int gameobject_ctf::on_player_death(class player *victim, class player *killer, int weaponid)
{
	gameobject::on_player_death(victim, killer, weaponid);
	int had_flag = 0;
	
	// drop flags
	for(int fi = 0; fi < 2; fi++)
	{
		flag *f = flags[fi];
		if(f && f->carrying_player == killer)
			had_flag |= 2;
		if(f && f->carrying_player == victim)
		{
			create_sound_global(SOUND_CTF_DROP);
			f->drop_tick = server_tick();
			f->carrying_player = 0;
			f->vel = vec2(0,0);
			
			if(killer && killer->team != victim->team)
				killer->score++;
				
			had_flag |= 1;
		}
	}
	
	return had_flag;
}

void gameobject_ctf::tick()
{
	gameobject::tick();

	do_team_score_wincheck();
	
	// do flags
	for(int fi = 0; fi < 2; fi++)
	{
		flag *f = flags[fi];
		
		if(!f)
			continue;
		
		//
		if(f->carrying_player)
		{
			// update flag position
			f->pos = f->carrying_player->pos;
			
			if(flags[fi^1]->at_stand)
			{
				if(distance(f->pos, flags[fi^1]->pos) < 32)
				{
					// CAPTURE! \o/
					teamscore[fi^1] += 100;
					f->carrying_player->score += 5;

					dbg_msg("game", "flag_capture player='%d:%s'", f->carrying_player->client_id, server_clientname(f->carrying_player->client_id));

					for(int i = 0; i < 2; i++)
						flags[i]->reset();
					
					create_sound_global(SOUND_CTF_CAPTURE);
				}
			}			
		}
		else
		{
			player *close_players[MAX_CLIENTS];
			int types[] = {NETOBJTYPE_PLAYER_CHARACTER};
			int num = world->find_entities(f->pos, 32.0f, (entity**)close_players, MAX_CLIENTS, types, 1);
			for(int i = 0; i < num; i++)
			{
				if(close_players[i]->team == f->team)
				{
					// return the flag
					if(!f->at_stand)
					{
						player *p = close_players[i];
						p->score += 1;

						dbg_msg("game", "flag_return player='%d:%s'", p->client_id, server_clientname(p->client_id));

						create_sound_global(SOUND_CTF_RETURN);
						f->reset();
					}
				}
				else
				{
					// take the flag
					if(f->at_stand)
						teamscore[fi^1]++;
					f->at_stand = 0;
					f->carrying_player = close_players[i];
					f->carrying_player->score += 1;

					dbg_msg("game", "flag_grab player='%d:%s'", f->carrying_player->client_id, server_clientname(f->carrying_player->client_id));
					
					for(int c = 0; c < MAX_CLIENTS; c++)
					{
						if(players[c].client_id == -1)
							continue;
							
						if(players[c].team == fi)
							create_sound_global(SOUND_CTF_GRAB_EN, players[c].client_id);
						else
							create_sound_global(SOUND_CTF_GRAB_PL, players[c].client_id);
					}
					break;
				}
			}
			
			if(!f->carrying_player && !f->at_stand)
			{
				if(server_tick() > f->drop_tick + server_tickspeed()*30)
				{
					create_sound_global(SOUND_CTF_RETURN);
					f->reset();
				}
				else
				{
					f->vel.y += world->core.tuning.gravity;
					move_box(&f->pos, &f->vel, vec2(f->phys_size, f->phys_size), 0.5f);
				}
			}
		}
	}
}

// Flag
flag::flag(int _team)
: entity(NETOBJTYPE_FLAG)
{
	team = _team;
	proximity_radius = phys_size;
	carrying_player = 0x0;
	
	reset();
	
	// TODO: should this be done here?
	world->insert_entity(this);
}

void flag::reset()
{
	carrying_player = 0;
	at_stand = 1;
	pos = stand_pos;
	vel = vec2(0,0);
}

void flag::snap(int snapping_client)
{
	NETOBJ_FLAG *flag = (NETOBJ_FLAG *)snap_new_item(NETOBJTYPE_FLAG, team, sizeof(NETOBJ_FLAG));
	flag->x = (int)pos.x;
	flag->y = (int)pos.y;
	flag->team = team;
	flag->carried_by = -1;
	
	if(at_stand)
		flag->carried_by = -2;
	else if(carrying_player)
		flag->carried_by = carrying_player->client_id;
}
