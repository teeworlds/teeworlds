#include <new>

#include <engine/e_server_interface.h>

#include "player.hpp"
#include "gamecontext.hpp"

PLAYER::PLAYER()
{
}

void PLAYER::init(int client_id)
{
	// clear everything
	mem_zero(this, sizeof(*this));
	new(this) PLAYER();
	this->client_id = client_id;
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
	
	if(spawning && !get_character())
		try_respawn();
	
	if(get_character())
		view_pos = get_character()->pos;
}

void PLAYER::snap(int snaping_client)
{
	NETOBJ_PLAYER_INFO *info = (NETOBJ_PLAYER_INFO *)snap_new_item(NETOBJTYPE_PLAYER_INFO, client_id, sizeof(NETOBJ_PLAYER_INFO));

	info->latency = latency.min;
	info->latency_flux = latency.max-latency.min;
	info->local = 0;
	info->cid = client_id;
	info->score = score;
	info->team = team;

	if(client_id == snaping_client)
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

	// clear this whole structure
	init(-1);
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
	if(character.alive)
		return &character;
	return 0;
}

void PLAYER::kill_character()
{
	CHARACTER *chr = get_character();
	if(chr)
		chr->die(-1, -1);
}

void PLAYER::respawn()
{
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
	
	game.controller->on_player_info_change(&game.players[client_id]);

	// send all info to this client
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i].client_id != -1)
			game.send_info(i, -1);
	}
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
		character.spawn(this, spawnpos, team);
	}
}
