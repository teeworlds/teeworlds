#include <engine/e_client_interface.h>

#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/layers.hpp>
#include "gc_render.hpp"

#include "gameclient.hpp"

#include "components/binds.hpp"
#include "components/broadcast.hpp"
#include "components/camera.hpp"
#include "components/chat.hpp"
#include "components/console.hpp"
#include "components/controls.hpp"
#include "components/damageind.hpp"
#include "components/debughud.hpp"
#include "components/effects.hpp"
#include "components/emoticon.hpp"
#include "components/flow.hpp"
#include "components/hud.hpp"
#include "components/items.hpp"
#include "components/killmessages.hpp"
#include "components/mapimages.hpp"
#include "components/maplayers.hpp"
#include "components/maplist.hpp"
#include "components/menus.hpp"
#include "components/motd.hpp"
#include "components/particles.hpp"
#include "components/players.hpp"
#include "components/nameplates.hpp"
#include "components/scoreboard.hpp"
#include "components/skins.hpp"
#include "components/sounds.hpp"
#include "components/voting.hpp"

GAMECLIENT gameclient;

// instanciate all systems
static KILLMESSAGES killmessages;
static CAMERA camera;
static CHAT chat;
static MOTD motd;
static BROADCAST broadcast;
static CONSOLE console;
static BINDS binds;
static PARTICLES particles;
static MENUS menus;
static SKINS skins;
static FLOW flow;
static HUD hud;
static DEBUGHUD debughud;
static CONTROLS controls;
static EFFECTS effects;
static SCOREBOARD scoreboard;
static SOUNDS sounds;
static EMOTICON emoticon;
static DAMAGEIND damageind;
static VOTING voting;

static PLAYERS players;
static NAMEPLATES nameplates;
static ITEMS items;
static MAPIMAGES mapimages;
static MAPLIST maplist;

static MAPLAYERS maplayers_background(MAPLAYERS::TYPE_BACKGROUND);
static MAPLAYERS maplayers_foreground(MAPLAYERS::TYPE_FOREGROUND);

GAMECLIENT::STACK::STACK() { num = 0; }
void GAMECLIENT::STACK::add(class COMPONENT *component) { components[num++] = component; }

static int load_current;
static int load_total;

static void load_sounds_thread(void *do_render)
{
	// load sounds
	for(int s = 0; s < data->num_sounds; s++)
	{
		if(do_render)
			gameclient.menus->render_loading(load_current/(float)load_total);
		for(int i = 0; i < data->sounds[s].num_sounds; i++)
		{
			int id = snd_load_wv(data->sounds[s].sounds[i].filename);
			data->sounds[s].sounds[i].id = id;
		}

		if(do_render)
			load_current++;
	}
}

void GAMECLIENT::on_console_init()
{
	// setup pointers
	binds = &::binds;
	console = &::console;
	particles = &::particles;
	menus = &::menus;
	skins = &::skins;
	chat = &::chat;
	flow = &::flow;
	camera = &::camera;
	controls = &::controls;
	effects = &::effects;
	sounds = &::sounds;
	motd = &::motd;
	damageind = &::damageind;
	mapimages = &::mapimages;
	voting = &::voting;
	maplist = &::maplist;
	
	// make a list of all the systems, make sure to add them in the corrent render order
	all.add(skins);
	all.add(mapimages);
	all.add(effects); // doesn't render anything, just updates effects
	all.add(particles);
	all.add(binds);
	all.add(controls);
	all.add(camera);
	all.add(sounds);
	all.add(voting);
	all.add(maplist);
	all.add(particles); // doesn't render anything, just updates all the particles
	
	all.add(&maplayers_background); // first to render
	all.add(&particles->render_trail);
	all.add(&particles->render_explosions);
	all.add(&items);
	all.add(&players);
	all.add(&maplayers_foreground);
	all.add(&nameplates);
	all.add(&particles->render_general);
	all.add(damageind);
	all.add(&hud);
	all.add(&emoticon);
	all.add(&killmessages);
	all.add(chat);
	all.add(&broadcast);
	all.add(&debughud);
	all.add(&scoreboard);
	all.add(motd);
	all.add(menus);
	all.add(console);
	
	// build the input stack
	input.add(&binds->special_binds);
	input.add(console);
	input.add(chat); // chat has higher prio due to tha you can quit it by pressing esc
	input.add(motd); // for pressing esc to remove it
	input.add(menus);
	input.add(&emoticon);
	input.add(controls);
	input.add(binds);
		
	// add the some console commands
	MACRO_REGISTER_COMMAND("team", "", con_team, this);
	MACRO_REGISTER_COMMAND("kill", "", con_kill, this);
	
	// let all the other components register their console commands
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_console_init();
}

void GAMECLIENT::on_init()
{
	// init all components
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_init();
	
	// setup item sizes
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		snap_set_staticsize(i, netobj_get_size(i));
	
	// load default font	
	static FONT_SET default_font;
	int64 start = time_get();
	
	int before = gfx_memory_usage();
	font_set_load(&default_font, "fonts/default_font%d.tfnt", "fonts/default_font%d.png", "fonts/default_font%d_b.png", 14, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 36);
	dbg_msg("font", "gfx memory used for font textures: %d", gfx_memory_usage()-before);
	
	gfx_text_set_default_font(&default_font);

	config.cl_threadsoundloading = 0;

	// setup load amount
	load_total = data->num_images;
	load_current = 0;
	if(!config.cl_threadsoundloading)
		load_total += data->num_sounds;
	
	// load textures
	for(int i = 0; i < data->num_images; i++)
	{
		gameclient.menus->render_loading(load_current/load_total);
		data->images[i].id = gfx_load_texture(data->images[i].filename, IMG_AUTO, 0);
		load_current++;
	}

	::skins.init();
	
	if(config.cl_threadsoundloading)
		thread_create(load_sounds_thread, 0);
	else
		load_sounds_thread((void*)1);
	
	int64 end = time_get();
	dbg_msg("", "%f.2ms", ((end-start)*1000)/(float)time_freq());
	
}

void GAMECLIENT::on_save()
{
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_save();
}

void GAMECLIENT::dispatch_input()
{
	// handle mouse movement
	int x=0, y=0;
	inp_mouse_relative(&x, &y);
	if(x || y)
	{
		for(int h = 0; h < input.num; h++)
		{
			if(input.components[h]->on_mousemove(x, y))
				break;
		}
	}
	
	// handle key presses
	for(int i = 0; i < inp_num_events(); i++)
	{
		INPUT_EVENT e = inp_get_event(i);
		
		for(int h = 0; h < input.num; h++)
		{
			if(input.components[h]->on_input(e))
			{
				//dbg_msg("", "%d char=%d key=%d flags=%d", h, e.ch, e.key, e.flags);
				break;
			}
		}
	}
	
	// clear all events for this frame
	inp_clear_events();	
}


int GAMECLIENT::on_snapinput(int *data)
{
	return controls->snapinput(data);
}

void GAMECLIENT::on_connected()
{
	layers_init();
	col_init();
	render_tilemap_generate_skip();

	for(int i = 0; i < all.num; i++)
		all.components[i]->on_mapload();
	
	// send the inital info
	send_info(true);
}

void GAMECLIENT::on_reset()
{
	// clear out the invalid pointers
	last_new_predicted_tick = -1;
	mem_zero(&gameclient.snap, sizeof(gameclient.snap));

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i].name[0] = 0;
		clients[i].skin_id = 0;
		clients[i].team = 0;
		clients[i].angle = 0;
		clients[i].emoticon = 0;
		clients[i].emoticon_start = -1;
		clients[i].skin_info.texture = gameclient.skins->get(0)->color_texture;
		clients[i].skin_info.color_body = vec4(1,1,1,1);
		clients[i].skin_info.color_feet = vec4(1,1,1,1);
		clients[i].update_render_info();
	}
	
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_reset();
}


void GAMECLIENT::update_local_character_pos()
{
	if(config.cl_predict)
	{
		if(!snap.local_character || (snap.local_character->health < 0) || (snap.gameobj && snap.gameobj->game_over))
		{
			// don't use predicted
		}
		else
			local_character_pos = mix(predicted_prev_char.pos, predicted_char.pos, client_predintratick());
	}
	else if(snap.local_character && snap.local_prev_character)
	{
		local_character_pos = mix(
			vec2(snap.local_prev_character->x, snap.local_prev_character->y),
			vec2(snap.local_character->x, snap.local_character->y), client_intratick());
	}
}


static void evolve(NETOBJ_CHARACTER *character, int tick)
{
	WORLD_CORE tempworld;
	CHARACTER_CORE tempcore;
	mem_zero(&tempcore, sizeof(tempcore));
	tempcore.world = &tempworld;
	tempcore.read(character);
	//tempcore.input.direction = character->wanted_direction;
	while(character->tick < tick)
	{
		character->tick++;
		tempcore.tick(false);
		tempcore.move();
		tempcore.quantize();
	}
	
	tempcore.write(character);
}


void GAMECLIENT::on_render()
{
	// perform dead reckoning
	// TODO: move this to a betterlocation
	/*
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!snap.characters[i].active)
			continue;
					
		// perform dead reckoning
		if(snap.characters[i].prev.tick)
			evolve(&snap.characters[i].prev, client_prevtick());
		if(snap.characters[i].cur.tick)
			evolve(&snap.characters[i].cur, client_tick());
	}*/
	
	// update the local character position
	update_local_character_pos();
	
	// dispatch all input to systems
	dispatch_input();
	
	// render all systems
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_render();
}

void GAMECLIENT::on_message(int msgtype)
{
	
	// special messages
	if(msgtype == NETMSGTYPE_SV_EXTRAPROJECTILE)
	{
		/*
		int num = msg_unpack_int();
		
		for(int k = 0; k < num; k++)
		{
			NETOBJ_PROJECTILE proj;
			for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
				((int *)&proj)[i] = msg_unpack_int();
				
			if(msg_unpack_error())
				return;
				
			if(extraproj_num != MAX_EXTRA_PROJECTILES)
			{
				extraproj_projectiles[extraproj_num] = proj;
				extraproj_num++;
			}
		}
		
		return;*/
	}
	else if(msgtype == NETMSGTYPE_SV_TUNEPARAMS)
	{
		// unpack the new tuning
		TUNING_PARAMS new_tuning;
		int *params = (int *)&new_tuning;
		for(unsigned i = 0; i < sizeof(TUNING_PARAMS)/sizeof(int); i++)
			params[i] = msg_unpack_int();

		// check for unpacking errors
		if(msg_unpack_error())
			return;
			
		// apply new tuning
		tuning = new_tuning;
		return;
	}
	
	void *rawmsg = netmsg_secure_unpack(msgtype);
	if(!rawmsg)
	{
		dbg_msg("client", "dropped weird message '%s' (%d), failed on '%s'", netmsg_get_name(msgtype), msgtype, netmsg_failed_on());
		return;
	}

	// TODO: this should be done smarter
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_message(msgtype, rawmsg);
	
	// handle core messages
	if(msgtype == NETMSGTYPE_SV_SETINFO)
	{
		NETMSG_SV_SETINFO *msg = (NETMSG_SV_SETINFO *)rawmsg;
		
		str_copy(clients[msg->cid].name, msg->name, 64);
		str_copy(clients[msg->cid].skin_name, msg->skin, 64);
		
		// make sure that we don't set a special skin on the client
		if(clients[msg->cid].skin_name[0] == 'x' || clients[msg->cid].skin_name[1] == '_')
			str_copy(clients[msg->cid].skin_name, "default", 64);
		
		clients[msg->cid].color_body = msg->color_body;
		clients[msg->cid].color_feet = msg->color_feet;
		
		clients[msg->cid].skin_info.color_body = skins->get_color(msg->color_body);
		clients[msg->cid].skin_info.color_feet = skins->get_color(msg->color_feet);
		clients[msg->cid].skin_info.size = 64;
		
		// find new skin
		clients[msg->cid].skin_id = gameclient.skins->find(clients[msg->cid].skin_name);
		if(clients[msg->cid].skin_id < 0)
			clients[msg->cid].skin_id = 0;
		
		clients[msg->cid].use_custom_color = msg->use_custom_color;
		
		if(msg->use_custom_color)
			clients[msg->cid].skin_info.texture = gameclient.skins->get(clients[msg->cid].skin_id)->color_texture;
		else
		{
			clients[msg->cid].skin_info.texture = gameclient.skins->get(clients[msg->cid].skin_id)->org_texture;
			clients[msg->cid].skin_info.color_body = vec4(1,1,1,1);
			clients[msg->cid].skin_info.color_feet = vec4(1,1,1,1);
		}

		clients[msg->cid].update_render_info();
	}
	else if(msgtype == NETMSGTYPE_SV_READYTOENTER)
	{
		client_entergame();
	}
	else if (msgtype == NETMSGTYPE_SV_EMOTICON)
	{
		NETMSG_SV_EMOTICON *msg = (NETMSG_SV_EMOTICON *)rawmsg;

		// apply
		clients[msg->cid].emoticon = msg->emoticon;
		clients[msg->cid].emoticon_start = client_tick();
	}
	else if(msgtype == NETMSGTYPE_SV_SOUNDGLOBAL)
	{
		NETMSG_SV_SOUNDGLOBAL *msg = (NETMSG_SV_SOUNDGLOBAL *)rawmsg;
		gameclient.sounds->play(SOUNDS::CHN_GLOBAL, msg->soundid, 1.0f, vec2(0,0));
	}		
}

void GAMECLIENT::on_statechange(int new_state, int old_state)
{
	// clear out the invalid pointers
	mem_zero(&gameclient.snap, sizeof(gameclient.snap));
		
	for(int i = 0; i < all.num; i++)
		all.components[i]->on_statechange(new_state, old_state);
}



void GAMECLIENT::process_events()
{
	int snaptype = SNAP_CURRENT;
	int num = snap_num_items(snaptype);
	for(int index = 0; index < num; index++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(snaptype, index, &item);

		if(item.type == NETEVENTTYPE_DAMAGEIND)
		{
			NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)data;
			gameclient.effects->damage_indicator(vec2(ev->x, ev->y), get_direction(ev->angle));
		}
		else if(item.type == NETEVENTTYPE_AIRJUMP)
		{
			NETEVENT_COMMON *ev = (NETEVENT_COMMON *)data;
			gameclient.effects->air_jump(vec2(ev->x, ev->y));
		}
		else if(item.type == NETEVENTTYPE_EXPLOSION)
		{
			NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)data;
			gameclient.effects->explosion(vec2(ev->x, ev->y));
		}
		else if(item.type == NETEVENTTYPE_SPAWN)
		{
			NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)data;
			gameclient.effects->playerspawn(vec2(ev->x, ev->y));
		}
		else if(item.type == NETEVENTTYPE_DEATH)
		{
			NETEVENT_DEATH *ev = (NETEVENT_DEATH *)data;
			gameclient.effects->playerdeath(vec2(ev->x, ev->y), ev->cid);
		}
		else if(item.type == NETEVENTTYPE_SOUNDWORLD)
		{
			NETEVENT_SOUNDWORLD *ev = (NETEVENT_SOUNDWORLD *)data;
			gameclient.sounds->play(SOUNDS::CHN_WORLD, ev->soundid, 1.0f, vec2(ev->x, ev->y));
		}
	}
}

void GAMECLIENT::on_snapshot()
{
	// clear out the invalid pointers
	mem_zero(&gameclient.snap, sizeof(gameclient.snap));
	snap.local_cid = -1;

	// secure snapshot
	{
		int num = snap_num_items(SNAP_CURRENT);
		for(int index = 0; index < num; index++)
		{
			SNAP_ITEM item;
			void *data = snap_get_item(SNAP_CURRENT, index, &item);
			if(netobj_validate(item.type, data, item.datasize) != 0)
			{
				if(config.debug)
					dbg_msg("game", "invalidated index=%d type=%d (%s) size=%d id=%d", index, item.type, netobj_get_name(item.type), item.datasize, item.id);
				snap_invalidate_item(SNAP_CURRENT, index);
			}
		}
	}
		
	process_events();

	if(config.dbg_stress)
	{
		if((client_tick()%250) == 0)
		{
			NETMSG_CL_SAY msg;
			msg.team = -1;
			msg.message = "galenskap!!!!";
			msg.pack(MSGFLAG_VITAL);
			client_send_msg();
		}
	}

	// go trough all the items in the snapshot and gather the info we want
	{
		snap.team_size[0] = snap.team_size[1] = 0;
		
		int num = snap_num_items(SNAP_CURRENT);
		for(int i = 0; i < num; i++)
		{
			SNAP_ITEM item;
			const void *data = snap_get_item(SNAP_CURRENT, i, &item);

			if(item.type == NETOBJTYPE_PLAYER_INFO)
			{
				const NETOBJ_PLAYER_INFO *info = (const NETOBJ_PLAYER_INFO *)data;
				
				clients[info->cid].team = info->team;
				snap.player_infos[info->cid] = info;
				
				if(info->local)
				{
					snap.local_cid = item.id;
					snap.local_info = info;
					
					if (info->team == -1)
						snap.spectate = true;
				}
				
				// calculate team-balance
				if(info->team != -1)
					snap.team_size[info->team]++;
				
			}
			else if(item.type == NETOBJTYPE_CHARACTER)
			{
				const void *old = snap_find_item(SNAP_PREV, NETOBJTYPE_CHARACTER, item.id);
				if(old)
				{
					snap.characters[item.id].active = true;
					snap.characters[item.id].prev = *((const NETOBJ_CHARACTER *)old);
					snap.characters[item.id].cur = *((const NETOBJ_CHARACTER *)data);
					
					if(snap.characters[item.id].prev.tick)
						evolve(&snap.characters[item.id].prev, client_prevtick());
					if(snap.characters[item.id].cur.tick)
						evolve(&snap.characters[item.id].cur, client_tick());
				}
			}
			else if(item.type == NETOBJTYPE_GAME)
				snap.gameobj = (NETOBJ_GAME *)data;
			else if(item.type == NETOBJTYPE_FLAG)
				snap.flags[item.id%2] = (const NETOBJ_FLAG *)data;
		}
	}
	
	if(client_state() == CLIENTSTATE_DEMOPLAYBACK)
		gameclient.snap.spectate = true;
	
	// setup local pointers
	if(snap.local_cid >= 0)
	{
		SNAPSTATE::CHARACTERINFO *c = &snap.characters[snap.local_cid];
		if(c->active)
		{
			snap.local_character = &c->cur;
			snap.local_prev_character = &c->prev;
			local_character_pos = vec2(snap.local_character->x, snap.local_character->y);
		}
	}

	// update render info
	for(int i = 0; i < MAX_CLIENTS; i++)
		clients[i].update_render_info();
}

void GAMECLIENT::on_predict()
{
	// store the previous values so we can detect prediction errors
	CHARACTER_CORE before_prev_char = predicted_prev_char;
	CHARACTER_CORE before_char = predicted_char;

	// we can't predict without our own id or own character
	if(snap.local_cid == -1 || !snap.characters[snap.local_cid].active)
		return;

	// repredict character
	WORLD_CORE world;
	world.tuning = tuning;

	// search for players
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!snap.characters[i].active)
			continue;
			
		gameclient.clients[i].predicted.world = &world;
		world.characters[i] = &gameclient.clients[i].predicted;
		gameclient.clients[i].predicted.read(&snap.characters[i].cur);
	}
	
	// predict
	for(int tick = client_tick()+1; tick <= client_predtick(); tick++)
	{
		// fetch the local
		if(tick == client_predtick() && world.characters[snap.local_cid])
			predicted_prev_char = *world.characters[snap.local_cid];
		
		// first calculate where everyone should move
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!world.characters[c])
				continue;

			mem_zero(&world.characters[c]->input, sizeof(world.characters[c]->input));
			if(snap.local_cid == c)
			{
				// apply player input
				int *input = client_get_input(tick);
				if(input)
					world.characters[c]->input = *((NETOBJ_PLAYER_INPUT*)input);
				world.characters[c]->tick(true);
			}
			else
				world.characters[c]->tick(false);

		}

		// move all players and quantize their data
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!world.characters[c])
				continue;

			world.characters[c]->move();
			world.characters[c]->quantize();
		}
		
		// check if we want to trigger effects
		if(tick > last_new_predicted_tick)
		{
			last_new_predicted_tick = tick;
			
			if(snap.local_cid != -1 && world.characters[snap.local_cid])
			{
				vec2 pos = world.characters[snap.local_cid]->pos;
				int events = world.characters[snap.local_cid]->triggered_events;
				if(events&COREEVENT_GROUND_JUMP) gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
				if(events&COREEVENT_AIR_JUMP)
				{
					gameclient.effects->air_jump(pos);
					gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, pos);
				}
				//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
				//if(events&COREEVENT_HOOK_ATTACH_PLAYER) snd_play_random(CHN_WORLD, SOUND_HOOK_ATTACH_PLAYER, 1.0f, pos);
				if(events&COREEVENT_HOOK_ATTACH_GROUND) gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_HOOK_ATTACH_GROUND, 1.0f, pos);
				if(events&COREEVENT_HOOK_HIT_NOHOOK) gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_HOOK_NOATTACH, 1.0f, pos);
				//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
			}
		}
		
		if(tick == client_predtick() && world.characters[snap.local_cid])
			predicted_char = *world.characters[snap.local_cid];
	}
	
	if(config.debug && config.cl_predict && predicted_tick == client_predtick())
	{
		NETOBJ_CHARACTER_CORE before = {0}, now = {0};
		before_char.write(&before);
		predicted_char.write(&now);

		if(mem_comp(&before, &now, sizeof(NETOBJ_CHARACTER_CORE)) != 0)
		{
			dbg_msg("client", "prediction error");
			for(unsigned i = 0; i < sizeof(NETOBJ_CHARACTER_CORE)/sizeof(int); i++)
				if(((int *)&before)[i] != ((int *)&now)[i])
				{
					dbg_msg("", "\t%d %d %d", i, ((int *)&before)[i], ((int *)&now)[i]);
				}
		}
	}
	
	predicted_tick = client_predtick();
}

void GAMECLIENT::CLIENT_DATA::update_render_info()
{
	render_info = skin_info;

	// force team colors
	if(gameclient.snap.gameobj && gameclient.snap.gameobj->flags&GAMEFLAG_TEAMS)
	{
		const int team_colors[2] = {65387, 10223467};
		if(team >= 0 || team <= 1)
		{
			render_info.texture = gameclient.skins->get(skin_id)->color_texture;
			render_info.color_body = gameclient.skins->get_color(team_colors[team]);
			render_info.color_feet = gameclient.skins->get_color(team_colors[team]);
		}
	}		
}

void GAMECLIENT::send_switch_team(int team)
{
	NETMSG_CL_SETTEAM msg;
	msg.team = team;
	msg.pack(MSGFLAG_VITAL);
	client_send_msg();	
}

void GAMECLIENT::send_info(bool start)
{
	if(start)
	{
		NETMSG_CL_STARTINFO msg;
		msg.name = config.player_name;
		msg.skin = config.player_skin;
		msg.use_custom_color = config.player_use_custom_color;
		msg.color_body = config.player_color_body;
		msg.color_feet = config.player_color_feet;
		msg.pack(MSGFLAG_VITAL|MSGFLAG_FLUSH);
	}
	else
	{
		NETMSG_CL_CHANGEINFO msg;
		msg.name = config.player_name;
		msg.skin = config.player_skin;
		msg.use_custom_color = config.player_use_custom_color;
		msg.color_body = config.player_color_body;
		msg.color_feet = config.player_color_feet;
		msg.pack(MSGFLAG_VITAL);
	}
	client_send_msg();
}

void GAMECLIENT::send_kill(int client_id)
{
	NETMSG_CL_KILL msg;
	msg.pack(MSGFLAG_VITAL);
	client_send_msg();
}

void GAMECLIENT::on_recordkeyframe()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!snap.player_infos[i])
			continue;
			
		NETMSG_SV_SETINFO msg;
		msg.cid = i;
		msg.name = clients[i].name;
		msg.skin = clients[i].skin_name;
		msg.use_custom_color = clients[i].use_custom_color;
		msg.color_body = clients[i].color_body;
		msg.color_feet = clients[i].color_feet;
		msg.pack(MSGFLAG_NOSEND|MSGFLAG_RECORD);
		client_send_msg();
	}
}

void GAMECLIENT::con_team(void *result, void *user_data)
{
	((GAMECLIENT*)user_data)->send_switch_team(console_arg_int(result, 0));
}

void GAMECLIENT::con_kill(void *result, void *user_data)
{
	((GAMECLIENT*)user_data)->send_kill(-1);
}
