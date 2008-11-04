/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_server_interface.h>
#include <game/mapitems.hpp>
#include <game/server/entities/character.hpp>
#include <game/server/player.hpp>
#include <game/server/gamecontext.hpp>
#include "ctf.hpp"

GAMECONTROLLER_CTF::GAMECONTROLLER_CTF()
{
	flags[0] = 0;
	flags[1] = 0;
	gametype = "CTF";
	game_flags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool GAMECONTROLLER_CTF::on_entity(int index, vec2 pos)
{
	if(GAMECONTROLLER::on_entity(index, pos))
		return true;
	
	int team = -1;
	if(index == ENTITY_FLAGSTAND_RED) team = 0;
	if(index == ENTITY_FLAGSTAND_BLUE) team = 1;
	if(team == -1)
		return false;
		
	FLAG *f = new FLAG(team);
	f->stand_pos = pos;
	f->pos = pos;
	flags[team] = f;
	return true;
}

int GAMECONTROLLER_CTF::on_character_death(class CHARACTER *victim, class PLAYER *killer, int weaponid)
{
	GAMECONTROLLER::on_character_death(victim, killer, weaponid);
	int had_flag = 0;
	
	// drop flags
	for(int fi = 0; fi < 2; fi++)
	{
		FLAG *f = flags[fi];
		if(f && killer && f->carrying_character == killer->get_character())
			had_flag |= 2;
		if(f && f->carrying_character == victim)
		{
			game.create_sound_global(SOUND_CTF_DROP);
			f->drop_tick = server_tick();
			f->carrying_character = 0;
			f->vel = vec2(0,0);
			
			if(killer && killer->team != victim->team)
				killer->score++;
				
			had_flag |= 1;
		}
	}
	
	return had_flag;
}

void GAMECONTROLLER_CTF::tick()
{
	GAMECONTROLLER::tick();

	do_team_score_wincheck();
	
	for(int fi = 0; fi < 2; fi++)
	{
		FLAG *f = flags[fi];
		
		if(!f)
			continue;
		
		// flag hits death-tile, reset it
		if(col_get((int)f->pos.x, (int)f->pos.y)&COLFLAG_DEATH)
		{
			f->reset();
			continue;
		}
		
		//
		if(f->carrying_character)
		{
			// update flag position
			f->pos = f->carrying_character->pos;
			
			if(flags[fi^1] && flags[fi^1]->at_stand)
			{
				if(distance(f->pos, flags[fi^1]->pos) < 32)
				{
					// CAPTURE! \o/
					teamscore[fi^1] += 100;
					f->carrying_character->player->score += 5;

					dbg_msg("game", "flag_capture player='%d:%s'",
						f->carrying_character->player->client_id,
						server_clientname(f->carrying_character->player->client_id));

					char buf[512];
					str_format(buf, sizeof(buf), "%s team has captured the flag!", fi^1 ? "Blue" : "Red");
					game.send_broadcast(buf, -1);
						
					for(int i = 0; i < 2; i++)
						flags[i]->reset();
					
					game.create_sound_global(SOUND_CTF_CAPTURE);
				}
			}			
		}
		else
		{
			CHARACTER *close_characters[MAX_CLIENTS];
			int num = game.world.find_entities(f->pos, 32.0f, (ENTITY**)close_characters, MAX_CLIENTS, NETOBJTYPE_CHARACTER);
			for(int i = 0; i < num; i++)
			{
				int collision = col_intersect_line(f->pos, close_characters[i]->pos, NULL);
				if(!collision && close_characters[i]->team == f->team)
				{
					// return the flag
					if(!f->at_stand)
					{
						CHARACTER *chr = close_characters[i];
						chr->player->score += 1;

						dbg_msg("game", "flag_return player='%d:%s'",
							chr->player->client_id,
							server_clientname(chr->player->client_id));

						game.create_sound_global(SOUND_CTF_RETURN);
						f->reset();
					}
				}
				else if(!collision)
				{
					// take the flag
					if(f->at_stand)
						teamscore[fi^1]++;
					f->at_stand = 0;
					f->carrying_character = close_characters[i];
					f->carrying_character->player->score += 1;

					dbg_msg("game", "flag_grab player='%d:%s'",
						f->carrying_character->player->client_id,
						server_clientname(f->carrying_character->player->client_id));
					
					for(int c = 0; c < MAX_CLIENTS; c++)
					{
						if(!game.players[c])
							continue;
							
						if(game.players[c]->team == fi)
							game.create_sound_global(SOUND_CTF_GRAB_EN, game.players[c]->client_id);
						else
							game.create_sound_global(SOUND_CTF_GRAB_PL, game.players[c]->client_id);
					}
					break;
				}
			}
			
			if(!f->carrying_character && !f->at_stand)
			{
				if(server_tick() > f->drop_tick + server_tickspeed()*30)
				{
					game.create_sound_global(SOUND_CTF_RETURN);
					f->reset();
				}
				else
				{
					f->vel.y += game.world.core.tuning.gravity;
					move_box(&f->pos, &f->vel, vec2(f->phys_size, f->phys_size), 0.5f);
				}
			}
		}
	}
}

// Flag
FLAG::FLAG(int _team)
: ENTITY(NETOBJTYPE_FLAG)
{
	team = _team;
	proximity_radius = phys_size;
	carrying_character = 0x0;
	
	reset();
	
	// TODO: should this be done here?
	game.world.insert_entity(this);
}

void FLAG::reset()
{
	carrying_character = 0x0;
	at_stand = 1;
	pos = stand_pos;
	vel = vec2(0,0);
}

void FLAG::snap(int snapping_client)
{
	NETOBJ_FLAG *flag = (NETOBJ_FLAG *)snap_new_item(NETOBJTYPE_FLAG, team, sizeof(NETOBJ_FLAG));
	flag->x = (int)pos.x;
	flag->y = (int)pos.y;
	flag->team = team;
	flag->carried_by = -1;
	
	if(at_stand)
		flag->carried_by = -2;
	else if(carrying_character && carrying_character->player)
		flag->carried_by = carrying_character->player->client_id;
}
