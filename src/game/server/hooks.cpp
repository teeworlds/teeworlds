/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <base/math.hpp>

#include <engine/e_config.h>
#include <engine/e_server_interface.h>
extern "C"
{
	#include <engine/e_memheap.h>
}
#include <game/version.hpp>
#include <game/collision.hpp>
#include <game/layers.hpp>

#include <game/gamecore.hpp>

#include "gamecontext.hpp"
#include "gamemodes/dm.hpp"
#include "gamemodes/tdm.hpp"
#include "gamemodes/ctf.hpp"
#include "gamemodes/mod.hpp"

TUNING_PARAMS tuning;

struct VOTEOPTION
{
	VOTEOPTION *next;
	VOTEOPTION *prev;
	char command[1];
};

static HEAP *voteoption_heap = 0;
static VOTEOPTION *voteoption_first = 0;
static VOTEOPTION *voteoption_last = 0;

void send_tuning_params(int cid)
{
	msg_pack_start(NETMSGTYPE_SV_TUNEPARAMS, MSGFLAG_VITAL);
	int *params = (int *)&tuning;
	for(unsigned i = 0; i < sizeof(tuning)/sizeof(int); i++)
		msg_pack_int(params[i]);
	msg_pack_end();
	server_send_msg(cid);
}

// Server hooks
void mods_client_direct_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id]->on_direct_input((NETOBJ_PLAYER_INPUT *)input);
}

void mods_client_predicted_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id]->on_predicted_input((NETOBJ_PLAYER_INPUT *)input);
}

// Server hooks
void mods_tick()
{
	game.tick();

	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			NETOBJ_PLAYER_INPUT input = {0};
			input.direction = (i&1)?-1:1;
			game.players[MAX_CLIENTS-i-1]->on_predicted_input(&input);
		}
	}
}

void mods_snap(int client_id)
{
	game.snap(client_id);
}

void mods_client_enter(int client_id)
{
	//game.world.insert_entity(&game.players[client_id]);
	game.players[client_id]->respawn();
	dbg_msg("game", "join player='%d:%s'", client_id, server_clientname(client_id));


	char buf[512];
	str_format(buf, sizeof(buf), "%s entered and joined the %s", server_clientname(client_id), game.controller->get_team_name(game.players[client_id]->team));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf); 

	dbg_msg("game", "team_join player='%d:%s' team=%d", client_id, server_clientname(client_id), game.players[client_id]->team);
}

void mods_connected(int client_id)
{
	game.players[client_id] = new(client_id) PLAYER(client_id);
	//game.players[client_id].init(client_id);
	//game.players[client_id].client_id = client_id;
	
	// Check which team the player should be on
	if(config.sv_tournament_mode)
		game.players[client_id]->team = -1;
	else
		game.players[client_id]->team = game.controller->get_auto_team(client_id);
		
	(void) game.controller->check_team_balance();

	// send motd
	NETMSG_SV_MOTD msg;
	msg.message = config.sv_motd;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(client_id);
}

void mods_client_drop(int client_id)
{
	game.abort_vote_kick_on_disconnect(client_id);
	game.players[client_id]->on_disconnect();
	delete game.players[client_id];
	game.players[client_id] = 0;
	
	(void) game.controller->check_team_balance();
}

/*static bool is_separator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

static const char *liststr_find(const char *str, const char *needle)
{
	int needle_len = strlen(needle);
	while(*str)
	{
		int wordlen = 0;
		while(str[wordlen] && !is_separator(str[wordlen]))
			wordlen++;
		
		if(wordlen == needle_len && strncmp(str, needle, needle_len) == 0)
			return str;
		
		str += wordlen+1;
	}
	
	return 0;
}*/

void mods_message(int msgtype, int client_id)
{
	void *rawmsg = netmsg_secure_unpack(msgtype);
	PLAYER *p = game.players[client_id];
	
	if(!rawmsg)
	{
		dbg_msg("server", "dropped weird message '%s' (%d), failed on '%s'", netmsg_get_name(msgtype), msgtype, netmsg_failed_on());
		return;
	}
	
	if(msgtype == NETMSGTYPE_CL_SAY)
	{
		NETMSG_CL_SAY *msg = (NETMSG_CL_SAY *)rawmsg;
		int team = msg->team;
		if(team)
			team = p->team;
		else
			team = GAMECONTEXT::CHAT_ALL;
		
		if(config.sv_spamprotection && p->last_chat+time_freq() > time_get())
			return;
		
		p->last_chat = time_get();
		
		game.send_chat(client_id, team, msg->message);
	}
	else if(msgtype == NETMSGTYPE_CL_CALLVOTE)
	{
		int64 now = time_get();
		if(game.vote_closetime)
		{
			game.send_chat(-1, client_id, "Wait for current vote to end before calling a new one.");
			return;
		}
		
		int64 timeleft = p->last_votecall + time_freq()*60 - now;
		if(timeleft > 0)
		{
			char chatmsg[512] = {0};
			str_format(chatmsg, sizeof(chatmsg), "You must wait %d seconds before making another vote", (timeleft/time_freq())+1);
			game.send_chat(-1, client_id, chatmsg);
			return;
		}
		
		char chatmsg[512] = {0};
		char desc[512] = {0};
		char cmd[512] = {0};
		NETMSG_CL_CALLVOTE *msg = (NETMSG_CL_CALLVOTE *)rawmsg;
		if(str_comp_nocase(msg->type, "option") == 0)
		{
			VOTEOPTION *option = voteoption_first;
			while(option)
			{
				if(str_comp_nocase(msg->value, option->command) == 0)
				{
					str_format(chatmsg, sizeof(chatmsg), "Vote called to change server option '%s'", option->command);
					str_format(desc, sizeof(desc), "%s", option->command);
					str_format(cmd, sizeof(cmd), "%s", option->command);
					break;
				}

				option = option->next;
			}
			
			if(!option)
			{
				str_format(chatmsg, sizeof(chatmsg), "'%s' isn't an option on this server", msg->value);
				game.send_chat(-1, client_id, chatmsg);
				return;
			}
		}
		else if(str_comp_nocase(msg->type, "kick") == 0)
		{
			if(!config.sv_vote_kick)
			{
				game.send_chat(-1, client_id, "Server does not allow voting to kick players");
				return;
			}
			
			int kick_id = atoi(msg->value);
			if(kick_id < 0 || kick_id >= MAX_CLIENTS || !game.players[kick_id])
			{
				game.send_chat(-1, client_id, "Invalid client id to kick");
				return;
			}
			
			str_format(chatmsg, sizeof(chatmsg), "Vote called to kick '%s'", server_clientname(kick_id));
			str_format(desc, sizeof(desc), "Kick '%s'", server_clientname(kick_id));
			str_format(cmd, sizeof(cmd), "kick %d", kick_id);
		}
		
		if(cmd[0])
		{
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, chatmsg);
			game.start_vote(desc, cmd);
			p->vote = 1;
			game.vote_creator = client_id;
			p->last_votecall = now;
			game.send_vote_status(-1);
		}
	}
	else if(msgtype == NETMSGTYPE_CL_VOTE)
	{
		if(!game.vote_closetime)
			return;

		if(p->vote == 0)
		{
			NETMSG_CL_VOTE *msg = (NETMSG_CL_VOTE *)rawmsg;
			p->vote = msg->vote;
			game.send_vote_status(-1);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_SETTEAM && !game.world.paused)
	{
		NETMSG_CL_SETTEAM *msg = (NETMSG_CL_SETTEAM *)rawmsg;
		
		if(config.sv_spamprotection && p->last_setteam+time_freq()*3 > time_get())
			return;

		// Switch team on given client and kill/respawn him
		if(game.controller->can_join_team(msg->team, client_id))
		{
			if(game.controller->can_change_team(p, msg->team))
			{
				p->last_setteam = time_get();
				p->set_team(msg->team);
				(void) game.controller->check_team_balance();
			}
			else
				game.send_broadcast("Teams must be balanced, please join other team", client_id);
		}
		else
		{
			char buf[128];
			str_format(buf, sizeof(buf), "Only %d active players are allowed", config.sv_max_clients-config.sv_spectator_slots);
			game.send_broadcast(buf, client_id);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_CHANGEINFO || msgtype == NETMSGTYPE_CL_STARTINFO)
	{
		NETMSG_CL_CHANGEINFO *msg = (NETMSG_CL_CHANGEINFO *)rawmsg;
		
		if(config.sv_spamprotection && p->last_changeinfo+time_freq()*5 > time_get())
			return;
			
		p->last_changeinfo = time_get();
		
		p->use_custom_color = msg->use_custom_color;
		p->color_body = msg->color_body;
		p->color_feet = msg->color_feet;

		// check for invalid chars
		unsigned char *name = (unsigned char *)msg->name;
		while (*name)
		{
			if(*name < 32)
				*name = ' ';
			name++;
		}

		// copy old name
		char oldname[MAX_NAME_LENGTH];
		str_copy(oldname, server_clientname(client_id), MAX_NAME_LENGTH);
		
		server_setclientname(client_id, msg->name);
		if(msgtype == NETMSGTYPE_CL_CHANGEINFO && strcmp(oldname, server_clientname(client_id)) != 0)
		{
			char chattext[256];
			str_format(chattext, sizeof(chattext), "%s changed name to %s", oldname, server_clientname(client_id));
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, chattext);
		}
		
		// set skin
		str_copy(p->skin_name, msg->skin, sizeof(p->skin_name));
		
		game.controller->on_player_info_change(p);
		
		if(msgtype == NETMSGTYPE_CL_STARTINFO)
		{
			// send vote options
			NETMSG_SV_VOTE_CLEAROPTIONS clearmsg;
			clearmsg.pack(MSGFLAG_VITAL);
			server_send_msg(client_id);
			VOTEOPTION *current = voteoption_first;
			while(current)
			{
				NETMSG_SV_VOTE_OPTION optionmsg;
				optionmsg.command = current->command;
				optionmsg.pack(MSGFLAG_VITAL);
				server_send_msg(client_id);
				current = current->next;
			}
			
			// send tuning parameters to client
			send_tuning_params(client_id);

			//
			NETMSG_SV_READYTOENTER m;
			m.pack(MSGFLAG_VITAL|MSGFLAG_FLUSH);
			server_send_msg(client_id);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_EMOTICON && !game.world.paused)
	{
		NETMSG_CL_EMOTICON *msg = (NETMSG_CL_EMOTICON *)rawmsg;
		
		if(config.sv_spamprotection && p->last_emote+time_freq()*5 > time_get())
			return;
			
		p->last_emote = time_get();
		
		game.send_emoticon(client_id, msg->emoticon);
	}
	else if (msgtype == NETMSGTYPE_CL_KILL && !game.world.paused)
	{
		if(p->last_kill+time_freq()*3 > time_get())
			return;
		
		p->last_kill = time_get();
		p->kill_character(); //(client_id, -1);
		p->respawn_tick = server_tick()+server_tickspeed()*3;
	}
}

static void con_tune_param(void *result, void *user_data)
{
	const char *param_name = console_arg_string(result, 0);
	float new_value = console_arg_float(result, 1);

	if(tuning.set(param_name, new_value))
	{
		dbg_msg("tuning", "%s changed to %.2f", param_name, new_value);
		send_tuning_params(-1);
	}
	else
		console_print("No such tuning parameter");
}

static void con_tune_reset(void *result, void *user_data)
{
	TUNING_PARAMS p;
	tuning = p;
	send_tuning_params(-1);
	console_print("tuning reset");
}

static void con_tune_dump(void *result, void *user_data)
{
	for(int i = 0; i < tuning.num(); i++)
	{
		float v;
		tuning.get(i, &v);
		dbg_msg("tuning", "%s %.2f", tuning.names[i], v);
	}
}


static void con_change_map(void *result, void *user_data)
{
	game.controller->change_map(console_arg_string(result, 0));
}

static void con_restart(void *result, void *user_data)
{
	if(console_arg_num(result))
		game.controller->do_warmup(console_arg_int(result, 0));
	else
		game.controller->startround();
}

static void con_broadcast(void *result, void *user_data)
{
	game.send_broadcast(console_arg_string(result, 0), -1);
}

static void con_say(void *result, void *user_data)
{
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, console_arg_string(result, 0));
}

static void con_set_team(void *result, void *user_data)
{
	int client_id = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS);
	int team = clamp(console_arg_int(result, 1), -1, 1);
	
	dbg_msg("", "%d %d", client_id, team);
	
	if(!game.players[client_id])
		return;
	
	game.players[client_id]->set_team(team);
	(void) game.controller->check_team_balance();
}

static void con_addvote(void *result, void *user_data)
{
	int len = strlen(console_arg_string(result, 0));
	
	if(!voteoption_heap)
		voteoption_heap = memheap_create();
	
	VOTEOPTION *option = (VOTEOPTION *)memheap_allocate(voteoption_heap, sizeof(VOTEOPTION) + len);
	option->next = 0;
	option->prev = voteoption_last;
	if(option->prev)
		option->prev->next = option;
	voteoption_last = option;
	if(!voteoption_first)
		voteoption_first = option;
	
	mem_copy(option->command, console_arg_string(result, 0), len+1);
	dbg_msg("server", "added option '%s'", option->command);
}

void mods_console_init()
{
	MACRO_REGISTER_COMMAND("tune", "si", con_tune_param, 0);
	MACRO_REGISTER_COMMAND("tune_reset", "", con_tune_reset, 0);
	MACRO_REGISTER_COMMAND("tune_dump", "", con_tune_dump, 0);

	MACRO_REGISTER_COMMAND("change_map", "r", con_change_map, 0);
	MACRO_REGISTER_COMMAND("restart", "?i", con_restart, 0);
	MACRO_REGISTER_COMMAND("broadcast", "r", con_broadcast, 0);
	MACRO_REGISTER_COMMAND("say", "r", con_say, 0);
	MACRO_REGISTER_COMMAND("set_team", "ii", con_set_team, 0);

	MACRO_REGISTER_COMMAND("addvote", "r", con_addvote, 0);
}

void mods_init()
{
	//if(!data) /* only load once */
		//data = load_data_from_memory(internal_data);
		
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		snap_set_staticsize(i, netobj_get_size(i));

	layers_init();
	col_init();

	// reset everything here
	//world = new GAMEWORLD;
	//players = new PLAYER[MAX_CLIENTS];

	// select gametype
	if(strcmp(config.sv_gametype, "mod") == 0)
		game.controller = new GAMECONTROLLER_MOD;
	else if(strcmp(config.sv_gametype, "ctf") == 0)
		game.controller = new GAMECONTROLLER_CTF;
	else if(strcmp(config.sv_gametype, "tdm") == 0)
		game.controller = new GAMECONTROLLER_TDM;
	else
		game.controller = new GAMECONTROLLER_DM;

	// setup core world
	//for(int i = 0; i < MAX_CLIENTS; i++)
	//	game.players[i].core.world = &game.world.core;

	// create all entities from the game layer
	MAPITEM_LAYER_TILEMAP *tmap = layers_game_layer();
	TILE *tiles = (TILE *)map_get_data(tmap->data);
	
	/*
	num_spawn_points[0] = 0;
	num_spawn_points[1] = 0;
	num_spawn_points[2] = 0;
	*/
	
	for(int y = 0; y < tmap->height; y++)
	{
		for(int x = 0; x < tmap->width; x++)
		{
			int index = tiles[y*tmap->width+x].index;
			
			if(index >= ENTITY_OFFSET)
			{
				vec2 pos(x*32.0f+16.0f, y*32.0f+16.0f);
				game.controller->on_entity(index-ENTITY_OFFSET, pos);
			}
		}
	}

	//game.world.insert_entity(game.controller);

	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			mods_connected(MAX_CLIENTS-i-1);
			mods_client_enter(MAX_CLIENTS-i-1);
			if(game.controller->is_teamplay())
				game.players[MAX_CLIENTS-i-1]->team = i&1;
		}
	}
}

void mods_shutdown()
{
	delete game.controller;
	game.controller = 0;
	game.clear();
}

void mods_presnap() {}
void mods_postsnap()
{
	game.events.clear();
}

extern "C" const char *mods_net_version() { return GAME_NETVERSION; }
extern "C" const char *mods_version() { return GAME_VERSION; }
