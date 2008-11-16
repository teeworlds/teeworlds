#include <new>

#include <engine/e_server_interface.h>

#include "player.hpp"
#include "gamecontext.hpp"

MACRO_ALLOC_POOL_ID_IMPL(PLAYER, MAX_CLIENTS)

PLAYER::PLAYER(int client_id)
{
	respawn_tick = server_tick();
	character = 0;
	this->client_id = client_id;
}

PLAYER::~PLAYER()
{
	delete character;
	character = 0;
}

void PLAYER::tick()
{
	server_setclientscore(client_id, score);

	// do latency stuff
	{
		CLIENT_INFO info;
		if(server_getclientinfo(client_id, &info))
		{
			latency.accum += info.latency;
			latency.accum_max = max(latency.accum_max, info.latency);
			latency.accum_min = min(latency.accum_min, info.latency);
		}

		if(server_tick()%server_tickspeed() == 0)
		{
			latency.avg = latency.accum/server_tickspeed();
			latency.max = latency.accum_max;
			latency.min = latency.accum_min;
			latency.accum = 0;
			latency.accum_min = 1000;
			latency.accum_max = 0;
		}
	}
	
	if(!character && die_tick+server_tickspeed()*3 <= server_tick())
		spawning = true;

	if(character)
	{
		if(character->alive)
		{
			view_pos = character->pos;
		}
		else
		{
			delete character;
			character = 0;
		}
	}
	else if(spawning && respawn_tick <= server_tick())
		try_respawn();
}

void PLAYER::snap(int snapping_client)
{
	NETOBJ_CLIENT_INFO *client_info = (NETOBJ_CLIENT_INFO *)snap_new_item(NETOBJTYPE_CLIENT_INFO, client_id, sizeof(NETOBJ_CLIENT_INFO));
	str_to_ints(&client_info->name0, 6, server_clientname(client_id));
	str_to_ints(&client_info->skin0, 6, skin_name);
	client_info->use_custom_color = use_custom_color;
	client_info->color_body = color_body;
	client_info->color_feet = color_feet;

	NETOBJ_PLAYER_INFO *info = (NETOBJ_PLAYER_INFO *)snap_new_item(NETOBJTYPE_PLAYER_INFO, client_id, sizeof(NETOBJ_PLAYER_INFO));

	info->latency = latency.min;
	info->latency_flux = latency.max-latency.min;
	info->local = 0;
	info->cid = client_id;
	info->score = score;
	info->team = team;

	if(client_id == snapping_client)
		info->local = 1;	
}

void PLAYER::on_disconnect()
{
	kill_character();
	
	//game.controller->on_player_death(&game.players[client_id], 0, -1);
		
	char buf[512];
	str_format(buf, sizeof(buf),  "%s has left the game", server_clientname(client_id));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);

	dbg_msg("game", "leave player='%d:%s'", client_id, server_clientname(client_id));
}

void PLAYER::on_predicted_input(NETOBJ_PLAYER_INPUT *new_input)
{
	CHARACTER *chr = get_character();
	if(chr)
		chr->on_predicted_input(new_input);
}

void PLAYER::on_direct_input(NETOBJ_PLAYER_INPUT *new_input)
{
	CHARACTER *chr = get_character();
	if(chr)
		chr->on_direct_input(new_input);

	if(!chr && team >= 0 && (new_input->fire&1))
	{
		spawning = true;
		dbg_msg("", "I wanna spawn");
	}
	
	if(!chr && team == -1)
		view_pos = vec2(new_input->target_x, new_input->target_y);
}

CHARACTER *PLAYER::get_character()
{
	if(character && character->alive)
		return character;
	return 0;
}

void PLAYER::kill_character()
{
	//CHARACTER *chr = get_character();
	if(character)
	{
		character->die(client_id, -1);
		delete character;
		character = 0;
	}
}

void PLAYER::respawn()
{
	if(team > -1)
		spawning = true;
}

void PLAYER::set_team(int new_team)
{
	// clamp the team
	new_team = game.controller->clampteam(new_team);
	if(team == new_team)
		return;
		
	char buf[512];
	str_format(buf, sizeof(buf), "%s joined the %s", server_clientname(client_id), game.controller->get_team_name(new_team));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf); 
	
	kill_character();
	team = new_team;
	score = 0;
	dbg_msg("game", "team_join player='%d:%s' team=%d", client_id, server_clientname(client_id), team);
	
	game.controller->on_player_info_change(game.players[client_id]);
}

void PLAYER::try_respawn()
{
	vec2 spawnpos = vec2(100.0f, -60.0f);
	
	if(!game.controller->can_spawn(this, &spawnpos))
		return;

	// check if the position is occupado
	ENTITY *ents[2] = {0};
	int num_ents = game.world.find_entities(spawnpos, 64, ents, 2, NETOBJTYPE_CHARACTER);
	
	if(num_ents == 0)
	{
		spawning = false;
		character = new(client_id) CHARACTER();
		character->spawn(this, spawnpos, team);
		game.create_playerspawn(spawnpos);
	}
}
