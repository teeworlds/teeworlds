#include <string.h>

#include <engine/e_client_interface.h>

extern "C" {
	#include <engine/e_config.h>
	#include <engine/client/ec_font.h>
	#include <engine/e_console.h>
};

#include <game/generated/gc_data.h>
#include <game/g_game.h>
#include <game/g_version.h>

#include <game/g_layers.h>

#include "gc_client.h"
#include "gc_skin.h"
#include "gc_render.h"
#include "gc_map_image.h"
#include "gc_console.h"

extern unsigned char internal_data[];

extern void menu_init();
extern bool menu_active;
extern bool menu_game_active;

extern "C" void modc_console_init()
{
	client_console_init();
}

extern "C" void modc_init()
{
	static FONT_SET default_font;

	int before = gfx_memory_usage();
	font_set_load(&default_font, "data/fonts/default_font%d.tfnt", "data/fonts/default_font%d.png", "data/fonts/default_font%d_b.png", 14, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 36);
	dbg_msg("font", "gfx memory used for font textures: %d", gfx_memory_usage()-before);
	
	gfx_text_set_default_font(&default_font);

	particle_reset();
	menu_init();
	
	// setup sound channels
	snd_set_channel(CHN_GUI, 1.0f, 0.0f);
	snd_set_channel(CHN_MUSIC, 1.0f, 0.0f);
	snd_set_channel(CHN_WORLD, 0.9f, 1.0f);
	snd_set_channel(CHN_GLOBAL, 1.0f, 0.0f);

	// load the data container
	data = load_data_from_memory(internal_data);

	// TODO: should be removed
	snd_set_listener_pos(0.0f, 0.0f);

	float total = data->num_sounds+data->num_images;
	float current = 0;

	// load textures
	for(int i = 0; i < data->num_images; i++)
	{
		render_loading(current/total);
		data->images[i].id = gfx_load_texture(data->images[i].filename, IMG_AUTO);
		current++;
	}
	
	// load sounds
	for(int s = 0; s < data->num_sounds; s++)
	{
		render_loading(current/total);
		for(int i = 0; i < data->sounds[s].num_sounds; i++)
		{
			int id;
			//if (strcmp(data->sounds[s].sounds[i].filename + strlen(data->sounds[s].sounds[i].filename) - 3, ".wv") == 0)
			id = snd_load_wv(data->sounds[s].sounds[i].filename);
			//else
			//	id = snd_load_wav(data->sounds[s].sounds[i].filename);

			data->sounds[s].sounds[i].id = id;
		}

		current++;
	}

	skin_init();
}

extern "C" void modc_entergame()
{
}

extern "C" void modc_shutdown()
{
	// shutdown the menu
}


player_core predicted_prev_player;
player_core predicted_player;
static int predicted_tick = 0;
static int last_new_predicted_tick = -1;

extern "C" void modc_predict()
{
	player_core before_prev_player = predicted_prev_player;
	player_core before_player = predicted_player;

	// repredict player
	world_core world;
	world.tuning = tuning;
	int local_cid = -1;

	// search for players
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);
		int client_id = item.id;

		if(item.type == OBJTYPE_PLAYER_CHARACTER)
		{
			const obj_player_character *character = (const obj_player_character *)data;
			client_datas[client_id].predicted.world = &world;
			world.players[client_id] = &client_datas[client_id].predicted;

			client_datas[client_id].predicted.read(character);
		}
		else if(item.type == OBJTYPE_PLAYER_INFO)
		{
			const obj_player_info *info = (const obj_player_info *)data;
			if(info->local)
				local_cid = client_id;
		}
	}

	// predict
	for(int tick = client_tick()+1; tick <= client_predtick(); tick++)
	{
		// fetch the local
		if(tick == client_predtick() && world.players[local_cid])
			predicted_prev_player = *world.players[local_cid];
		
		// first calculate where everyone should move
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!world.players[c])
				continue;

			mem_zero(&world.players[c]->input, sizeof(world.players[c]->input));
			if(local_cid == c)
			{
				// apply player input
				int *input = client_get_input(tick);
				if(input)
					world.players[c]->input = *((player_input*)input);
			}

			world.players[c]->tick();
		}

		// move all players and quantize their data
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!world.players[c])
				continue;

			world.players[c]->move();
			world.players[c]->quantize();
		}
		
		if(tick > last_new_predicted_tick)
		{
			last_new_predicted_tick = tick;
			
			if(local_cid != -1 && world.players[local_cid])
			{
				vec2 pos = world.players[local_cid]->pos;
				int events = world.players[local_cid]->triggered_events;
				if(events&COREEVENT_GROUND_JUMP) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
				if(events&COREEVENT_AIR_JUMP)
				{
					effect_air_jump(pos);
					snd_play_random(CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, pos);
				}
				//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
				//if(events&COREEVENT_HOOK_ATTACH_PLAYER) snd_play_random(CHN_WORLD, SOUND_HOOK_ATTACH_PLAYER, 1.0f, pos);
				if(events&COREEVENT_HOOK_ATTACH_GROUND) snd_play_random(CHN_WORLD, SOUND_HOOK_ATTACH_GROUND, 1.0f, pos);
				//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
			}


			/*
			dbg_msg("predict", "%d %d %d", tick,
				(int)world.players[c]->pos.x, (int)world.players[c]->pos.y,
				(int)world.players[c]->vel.x, (int)world.players[c]->vel.y);*/
		}
		
		if(tick == client_predtick() && world.players[local_cid])
			predicted_player = *world.players[local_cid];
	}
	
	if(config.debug && predicted_tick == client_predtick())
	{
		if(predicted_player.pos.x != before_player.pos.x ||
			predicted_player.pos.y != before_player.pos.y)
		{
			dbg_msg("client", "prediction error, (%d %d) (%d %d)", 
				(int)before_player.pos.x, (int)before_player.pos.y,
				(int)predicted_player.pos.x, (int)predicted_player.pos.y);
		}

		if(predicted_prev_player.pos.x != before_prev_player.pos.x ||
			predicted_prev_player.pos.y != before_prev_player.pos.y)
		{
			dbg_msg("client", "prediction error, prev (%d %d) (%d %d)", 
				(int)before_prev_player.pos.x, (int)before_prev_player.pos.y,
				(int)predicted_prev_player.pos.x, (int)predicted_prev_player.pos.y);
		}
	}
	
	predicted_tick = client_predtick();
}


extern "C" void modc_newsnapshot()
{
	static int snapshot_count = 0;
	snapshot_count++;
	
	process_events(SNAP_CURRENT);

	if(config.dbg_stress)
	{
		if((client_tick()%250) == 0)
		{
			msg_pack_start(MSG_SAY, MSGFLAG_VITAL);
			msg_pack_int(-1);
			msg_pack_string("galenskap!!!!", 512);
			msg_pack_end();
			client_send_msg();
		}
	}

	clear_object_pointers();

	// setup world view
	{
		// 1. fetch local player
		// 2. set him to the center
		int num = snap_num_items(SNAP_CURRENT);
		for(int i = 0; i < num; i++)
		{
			SNAP_ITEM item;
			const void *data = snap_get_item(SNAP_CURRENT, i, &item);

			if(item.type == OBJTYPE_PLAYER_INFO)
			{
				const obj_player_info *info = (const obj_player_info *)data;
				
				client_datas[info->clientid].team = info->team;
				
				if(info->local)
				{
					local_info = info;
					const void *data = snap_find_item(SNAP_CURRENT, OBJTYPE_PLAYER_CHARACTER, item.id);
					if(data)
					{
						local_character = (const obj_player_character *)data;
						local_character_pos = vec2(local_character->x, local_character->y);

						const void *p = snap_find_item(SNAP_PREV, OBJTYPE_PLAYER_CHARACTER, item.id);
						if(p)
							local_prev_character = (obj_player_character *)p;
					}
				}
			}
			else if(item.type == OBJTYPE_GAME)
				gameobj = (obj_game *)data;
			else if(item.type == OBJTYPE_FLAG)
			{
				flags[item.id%2] = (const obj_flag *)data;
			}
		}
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
		client_datas[i].update_render_info();
}

extern "C" void modc_render()
{
	console_handle_input();

	// this should be moved around abit
	if(client_state() == CLIENTSTATE_ONLINE)
	{
		render_game();
	}
	else
	{
		menu_render();
	}

	console_render();
}

extern "C" int modc_snap_input(int *data)
{
	picked_up_weapon = -1;

	if(!input_target_lock)
	{
		input_data.target_x = (int)mouse_pos.x;
		input_data.target_y = (int)mouse_pos.y;
		
		if(!input_data.target_x && !input_data.target_y)
			input_data.target_y = 1;
	}
	input_target_lock = 0;

	if(chat_mode != CHATMODE_NONE)
		input_data.player_state = PLAYERSTATE_CHATTING;
	else if(menu_active)
		input_data.player_state = PLAYERSTATE_IN_MENU;
	else
	{
		input_data.player_state = PLAYERSTATE_PLAYING;

		// TODO: this doesn't feel too pretty... look into it?
		if (console_active())
		{
			input_data.left = 0;
			input_data.right = 0;
			input_data.hook = 0;
			input_data.jump = 0;
		}
		else
		{
			input_data.left = inp_key_state(config.key_move_left);
			input_data.right = inp_key_state(config.key_move_right);
			input_data.hook = inp_key_state(config.key_hook);
			input_data.jump  = inp_key_state(config.key_jump);
		}
	}

	// stress testing
	if(config.dbg_stress)
	{
		float t = client_localtime();
		mem_zero(&input_data, sizeof(input_data));

		input_data.left = ((int)t/2)&1;
		input_data.right = ((int)t/2+1)&1;
		input_data.jump = ((int)t);
		input_data.fire = ((int)(t*10));
		input_data.hook = ((int)(t*2))&1;
		input_data.wanted_weapon = ((int)t)%NUM_WEAPONS;
		input_data.target_x = (int)(sinf(t*3)*100.0f);
		input_data.target_y = (int)(cosf(t*3)*100.0f);
	}

	// copy and return size	
	mem_copy(data, &input_data, sizeof(input_data));
	return sizeof(input_data);
}

void menu_do_disconnected();
void menu_do_connecting();
void menu_do_connected();

extern "C" void modc_statechange(int state, int old)
{
	clear_object_pointers();
	
	if(state == CLIENTSTATE_OFFLINE)
	{
	 	menu_do_disconnected();
	 	menu_game_active = false;
	}
	else if(state == CLIENTSTATE_LOADING)
		menu_do_connecting();
	else if(state == CLIENTSTATE_CONNECTING)
		menu_do_connecting();
	else if (state == CLIENTSTATE_ONLINE)
	{
		menu_active = false;
	 	menu_game_active = true;
	 	//snapshot_count = 0;
	 	
		menu_do_connected();
	}
}


obj_projectile extraproj_projectiles[MAX_EXTRA_PROJECTILES];
int extraproj_num;

void extraproj_reset()
{
	extraproj_num = 0;
}

extern "C" void modc_message(int msg)
{
	if(msg == MSG_CHAT)
	{
		int cid = msg_unpack_int();
		int team = msg_unpack_int();
		const char *message = msg_unpack_string();
		dbg_msg("message", "chat cid=%d team=%d msg='%s'", cid, team, message);
		chat_add_line(cid, team, message);

		if(cid >= 0)
			snd_play(CHN_GUI, data->sounds[SOUND_CHAT_CLIENT].sounds[0].id, 0);
		else
			snd_play(CHN_GUI, data->sounds[SOUND_CHAT_SERVER].sounds[0].id, 0);
	}
	else if(msg == MSG_EXTRA_PROJECTILE)
	{
		int num = msg_unpack_int();
		
		for(int k = 0; k < num; k++)
		{
			obj_projectile proj;
			for(unsigned i = 0; i < sizeof(obj_projectile)/sizeof(int); i++)
				((int *)&proj)[i] = msg_unpack_int();
				
			if(extraproj_num != MAX_EXTRA_PROJECTILES)
			{
				extraproj_projectiles[extraproj_num] = proj;
				extraproj_num++;
			}
		}
	}
	else if(msg == MSG_SETINFO)
	{
		int cid = msg_unpack_int();
		const char *name = msg_unpack_string();
		const char *skinname = msg_unpack_string();
		
		strncpy(client_datas[cid].name, name, 64);
		strncpy(client_datas[cid].skin_name, skinname, 64);
		
		int use_custom_color = msg_unpack_int();
		client_datas[cid].skin_info.color_body = skin_get_color(msg_unpack_int());
		client_datas[cid].skin_info.color_feet = skin_get_color(msg_unpack_int());
		client_datas[cid].skin_info.size = 64;
		
		// find new skin
		client_datas[cid].skin_id = skin_find(client_datas[cid].skin_name);
		if(client_datas[cid].skin_id < 0)
			client_datas[cid].skin_id = 0;
		
		if(use_custom_color)
			client_datas[cid].skin_info.texture = skin_get(client_datas[cid].skin_id)->color_texture;
		else
		{
			client_datas[cid].skin_info.texture = skin_get(client_datas[cid].skin_id)->org_texture;
			client_datas[cid].skin_info.color_body = vec4(1,1,1,1);
			client_datas[cid].skin_info.color_feet = vec4(1,1,1,1);
		}
		
		client_datas[cid].update_render_info();
	}
	else if(msg == MSG_TUNE_PARAMS)
	{
		int *params = (int *)&tuning;
		for(unsigned i = 0; i < sizeof(tuning_params)/sizeof(int); i++)
			params[i] = msg_unpack_int();
	}
    else if(msg == MSG_WEAPON_PICKUP)
    {
        int weapon = msg_unpack_int();
        picked_up_weapon = weapon+1;
    }
	else if(msg == MSG_READY_TO_ENTER)
	{
		client_entergame();
	}
	else if(msg == MSG_KILLMSG)
	{
		killmsg_current = (killmsg_current+1)%killmsg_max;
		killmsgs[killmsg_current].killer = msg_unpack_int();
		killmsgs[killmsg_current].victim = msg_unpack_int();
		killmsgs[killmsg_current].weapon = msg_unpack_int();
		killmsgs[killmsg_current].mode_special = msg_unpack_int();
		killmsgs[killmsg_current].tick = client_tick();
	}
	else if (msg == MSG_EMOTICON)
	{
		int cid = msg_unpack_int();
		int emoticon = msg_unpack_int();
		client_datas[cid].emoticon = emoticon;
		client_datas[cid].emoticon_start = client_tick();
	}
	else if(msg == MSG_SOUND_GLOBAL)
	{
		int soundid = msg_unpack_int();
		snd_play_random(CHN_GLOBAL, soundid, 1.0f, vec2(0,0));
	}
}

extern "C" void modc_connected()
{
	// init some stuff
	layers_init();
	col_init();
	img_init();
	flow_init();
	
	//tilemap_init();
	chat_reset();
	particle_reset();
	extraproj_reset();
	
	clear_object_pointers();
	last_new_predicted_tick = -1;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		client_datas[i].name[0] = 0;
		client_datas[i].team = 0;
		client_datas[i].emoticon = 0;
		client_datas[i].emoticon_start = -1;
	}

	for(int i = 0; i < killmsg_max; i++)
		killmsgs[i].tick = -100000;
		
	send_info(true);
}

extern "C" const char *modc_net_version() { return TEEWARS_NETVERSION; }
