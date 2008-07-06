/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
#include <game/g_version.hpp>
#include <game/g_collision.hpp>
#include <game/g_layers.hpp>
#include <game/g_math.hpp>
#include "gs_common.hpp"
#include "gs_game_ctf.hpp"
#include "gs_game_tdm.hpp"
#include "gs_game_dm.hpp"

TUNING_PARAMS tuning;
GAMECONTEXT game;

void GAMECONTEXT::send_chat(int chatter_cid, int team, const char *text)
{
	if(chatter_cid >= 0 && chatter_cid < MAX_CLIENTS)
		dbg_msg("chat", "%d:%d:%s: %s", chatter_cid, team, server_clientname(chatter_cid), text);
	else
		dbg_msg("chat", "*** %s", text);

	if(team == CHAT_ALL)
	{
		NETMSG_SV_CHAT msg;
		msg.team = 0;
		msg.cid = chatter_cid;
		msg.message = text;
		msg.pack(MSGFLAG_VITAL);
		server_send_msg(-1);
	}
	else
	{
		NETMSG_SV_CHAT msg;
		msg.team = 1;
		msg.cid = chatter_cid;
		msg.message = text;
		msg.pack(MSGFLAG_VITAL);

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(game.players[i].client_id != -1 && game.players[i].team == team)
				server_send_msg(i);
		}
	}
}


void GAMECONTEXT::send_info(int who, int to_who)
{
	NETMSG_SV_SETINFO msg;
	msg.cid = who;
	msg.name = server_clientname(who);
	msg.skin = players[who].skin_name;
	msg.use_custom_color = players[who].use_custom_color;
	msg.color_body = players[who].color_body;
	msg.color_feet = players[who].color_feet;
	msg.pack(MSGFLAG_VITAL);
	
	server_send_msg(to_who);
}

void GAMECONTEXT::send_emoticon(int cid, int emoticon)
{
	NETMSG_SV_EMOTICON msg;
	msg.cid = cid;
	msg.emoticon = emoticon;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(-1);
}

void GAMECONTEXT::send_weapon_pickup(int cid, int weapon)
{
	NETMSG_SV_WEAPONPICKUP msg;
	msg.weapon = weapon;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(cid);
}


void GAMECONTEXT::send_broadcast(const char *text, int cid)
{
	NETMSG_SV_BROADCAST msg;
	msg.message = text;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(cid);
}

void send_tuning_params(int cid)
{
	/*
	msg_pack_start(NETMSGTYPE_SV_TUNE_PARAMS, MSGFLAG_VITAL);
	int *params = (int *)&tuning;
	for(unsigned i = 0; i < sizeof(tuning_params)/sizeof(int); i++)
		msg_pack_int(params[i]);
	msg_pack_end();
	server_send_msg(cid);
	*/
}

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
EVENT_HANDLER::EVENT_HANDLER()
{
	clear();
}

void *EVENT_HANDLER::create(int type, int size, int mask)
{
	if(num_events == MAX_EVENTS)
		return 0;
	if(current_offset+size >= MAX_DATASIZE)
		return 0;

	void *p = &data[current_offset];
	offsets[num_events] = current_offset;
	types[num_events] = type;
	sizes[num_events] = size;
	client_masks[num_events] = mask;
	current_offset += size;
	num_events++;
	return p;
}

void EVENT_HANDLER::clear()
{
	num_events = 0;
	current_offset = 0;
}

void EVENT_HANDLER::snap(int snapping_client)
{
	for(int i = 0; i < num_events; i++)
	{
		if(cmask_is_set(client_masks[i], snapping_client))
		{
			NETEVENT_COMMON *ev = (NETEVENT_COMMON *)&data[offsets[i]];
			if(distance(game.players[snapping_client].view_pos, vec2(ev->x, ev->y)) < 1500.0f)
			{
				void *d = snap_new_item(types[i], i, sizes[i]);
				mem_copy(d, &data[offsets[i]], sizes[i]);
			}
		}
	}
}

//////////////////////////////////////////////////
// Entity
//////////////////////////////////////////////////
ENTITY::ENTITY(int objtype)
{
	this->objtype = objtype;
	pos = vec2(0,0);
	proximity_radius = 0;

	marked_for_destroy = false;	
	id = snap_new_id();

	next_entity = 0;
	prev_entity = 0;
	prev_type_entity = 0;
	next_type_entity = 0;
}

ENTITY::~ENTITY()
{
	game.world.remove_entity(this);
	snap_free_id(id);
}

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
GAMEWORLD::GAMEWORLD()
{
	paused = false;
	reset_requested = false;
	first_entity = 0x0;
	for(int i = 0; i < NUM_ENT_TYPES; i++)
		first_entity_types[i] = 0;
}

GAMEWORLD::~GAMEWORLD()
{
	// delete all entities
	while(first_entity)
		delete first_entity;
}

ENTITY *GAMEWORLD::find_first(int type)
{
	return first_entity_types[type];
}


int GAMEWORLD::find_entities(vec2 pos, float radius, ENTITY **ents, int max, int type)
{
	int num = 0;
	for(ENTITY *ent = (type<0) ? first_entity : first_entity_types[type];
		ent; ent = (type<0) ? ent->next_entity : ent->next_type_entity)
	{
		if(distance(ent->pos, pos) < radius+ent->proximity_radius)
		{
			ents[num] = ent;
			num++;
			if(num == max)
				break;
		}
	}

	return num;
}

void GAMEWORLD::insert_entity(ENTITY *ent)
{
	ENTITY *cur = first_entity;
	while(cur)
	{
		dbg_assert(cur != ent, "err");
		cur = cur->next_entity;
	}

	// insert it
	if(first_entity)
		first_entity->prev_entity = ent;
	ent->next_entity = first_entity;
	ent->prev_entity = 0x0;
	first_entity = ent;

	// into typelist aswell
	if(first_entity_types[ent->objtype])
		first_entity_types[ent->objtype]->prev_type_entity = ent;
	ent->next_type_entity = first_entity_types[ent->objtype];
	ent->prev_type_entity = 0x0;
	first_entity_types[ent->objtype] = ent;
}

void GAMEWORLD::destroy_entity(ENTITY *ent)
{
	ent->marked_for_destroy = true;
}

void GAMEWORLD::remove_entity(ENTITY *ent)
{
	// not in the list
	if(!ent->next_entity && !ent->prev_entity && first_entity != ent)
		return;

	// remove
	if(ent->prev_entity)
		ent->prev_entity->next_entity = ent->next_entity;
	else
		first_entity = ent->next_entity;
	if(ent->next_entity)
		ent->next_entity->prev_entity = ent->prev_entity;

	if(ent->prev_type_entity)
		ent->prev_type_entity->next_type_entity = ent->next_type_entity;
	else
		first_entity_types[ent->objtype] = ent->next_type_entity;
	if(ent->next_type_entity)
		ent->next_type_entity->prev_type_entity = ent->prev_type_entity;

	ent->next_entity = 0;
	ent->prev_entity = 0;
	ent->next_type_entity = 0;
	ent->prev_type_entity = 0;
}

//
void GAMEWORLD::snap(int snapping_client)
{
	for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
		ent->snap(snapping_client);
}

void GAMEWORLD::reset()
{
	// reset all entities
	for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
		ent->reset();
	remove_entities();

	game.controller->post_reset();
	remove_entities();

	reset_requested = false;
}

void GAMEWORLD::remove_entities()
{
	// destroy objects marked for destruction
	ENTITY *ent = first_entity;
	while(ent)
	{
		ENTITY *next = ent->next_entity;
		if(ent->marked_for_destroy)
		{
			remove_entity(ent);
			ent->destroy();
		}
		ent = next;
	}
}

void GAMEWORLD::tick()
{
	if(reset_requested)
		reset();

	if(!paused)
	{
		// update all objects
		for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick();
		
		for(ENTITY *ent = first_entity; ent; ent = ent->next_entity)
			ent->tick_defered();
	}

	remove_entities();
}

//////////////////////////////////////////////////
// projectile
//////////////////////////////////////////////////
PROJECTILE::PROJECTILE(int type, int owner, vec2 pos, vec2 dir, int span, ENTITY* powner,
	int damage, int flags, float force, int sound_impact, int weapon)
: ENTITY(NETOBJTYPE_PROJECTILE)
{
	this->type = type;
	this->pos = pos;
	this->direction = dir;
	this->lifespan = span;
	this->owner = owner;
	this->powner = powner;
	this->flags = flags;
	this->force = force;
	this->damage = damage;
	this->sound_impact = sound_impact;
	this->weapon = weapon;
	this->bounce = 0;
	this->start_tick = server_tick();
	game.world.insert_entity(this);
}

void PROJECTILE::reset()
{
	game.world.destroy_entity(this);
}

vec2 PROJECTILE::get_pos(float time)
{
	float curvature = 0;
	float speed = 0;
	if(type == WEAPON_GRENADE)
	{
		curvature = tuning.grenade_curvature;
		speed = tuning.grenade_speed;
	}
	else if(type == WEAPON_SHOTGUN)
	{
		curvature = tuning.shotgun_curvature;
		speed = tuning.shotgun_speed;
	}
	else if(type == WEAPON_GUN)
	{
		curvature = tuning.gun_curvature;
		speed = tuning.gun_speed;
	}
	
	return calc_pos(pos, direction, curvature, speed, time);
}


void PROJECTILE::tick()
{
	
	float pt = (server_tick()-start_tick-1)/(float)server_tickspeed();
	float ct = (server_tick()-start_tick)/(float)server_tickspeed();
	vec2 prevpos = get_pos(pt);
	vec2 curpos = get_pos(ct);

	lifespan--;
	
	int collide = col_intersect_line(prevpos, curpos, &curpos);
	//int collide = col_check_point((int)curpos.x, (int)curpos.y);
	
	CHARACTER *targetchr = game.world.intersect_character(prevpos, curpos, 6.0f, curpos, powner);
	if(targetchr || collide || lifespan < 0)
	{
		if(lifespan >= 0 || weapon == WEAPON_GRENADE)
			game.create_sound(curpos, sound_impact);

		if(flags & PROJECTILE_FLAGS_EXPLODE)
			game.create_explosion(curpos, owner, weapon, false);
		else if(targetchr)
		{
			targetchr->take_damage(direction * max(0.001f, force), damage, owner, weapon);
		}

		game.world.destroy_entity(this);
	}
}

void PROJECTILE::fill_info(NETOBJ_PROJECTILE *proj)
{
	proj->x = (int)pos.x;
	proj->y = (int)pos.y;
	proj->vx = (int)(direction.x*100.0f);
	proj->vy = (int)(direction.y*100.0f);
	proj->start_tick = start_tick;
	proj->type = type;
}

void PROJECTILE::snap(int snapping_client)
{
	float ct = (server_tick()-start_tick)/(float)server_tickspeed();
	
	if(distance(game.players[snapping_client].view_pos, get_pos(ct)) > 1000.0f)
		return;

	NETOBJ_PROJECTILE *proj = (NETOBJ_PROJECTILE *)snap_new_item(NETOBJTYPE_PROJECTILE, id, sizeof(NETOBJ_PROJECTILE));
	fill_info(proj);
}


//////////////////////////////////////////////////
// laser
//////////////////////////////////////////////////
LASER::LASER(vec2 pos, vec2 direction, float start_energy, CHARACTER *owner)
: ENTITY(NETOBJTYPE_LASER)
{
	this->pos = pos;
	this->owner = owner;
	energy = start_energy;
	dir = direction;
	bounces = 0;
	do_bounce();
	
	game.world.insert_entity(this);
}


bool LASER::hit_character(vec2 from, vec2 to)
{
	vec2 at;
	CHARACTER *hit = game.world.intersect_character(pos, to, 0.0f, at, owner);
	if(!hit)
		return false;

	this->from = from;
	pos = at;
	energy = -1;		
	hit->take_damage(vec2(0,0), tuning.laser_damage, owner->player->client_id, WEAPON_RIFLE);
	return true;
}

void LASER::do_bounce()
{
	eval_tick = server_tick();
	
	if(energy < 0)
	{
		//dbg_msg("laser", "%d removed", server_tick());
		game.world.destroy_entity(this);
		return;
	}
	
	vec2 to = pos + dir*energy;
	
	if(col_intersect_line(pos, to, &to))
	{
		if(!hit_character(pos, to))
		{
			// intersected
			from = pos;
			pos = to - dir*2;
			vec2 temp_pos = pos;
			vec2 temp_dir = dir*4.0f;
			
			move_point(&temp_pos, &temp_dir, 1.0f, 0);
			pos = temp_pos;
			dir = normalize(temp_dir);
			
			energy -= distance(from, pos) + tuning.laser_bounce_cost;
			bounces++;
			
			if(bounces > tuning.laser_bounce_num)
				energy = -1;
				
			game.create_sound(pos, SOUND_RIFLE_BOUNCE);
		}
	}
	else
	{
		if(!hit_character(pos, to))
		{
			from = pos;
			pos = to;
			energy = -1;
		}
	}
		
	//dbg_msg("laser", "%d done %f %f %f %f", server_tick(), from.x, from.y, pos.x, pos.y);
}
	
void LASER::reset()
{
	game.world.destroy_entity(this);
}

void LASER::tick()
{
	if(server_tick() > eval_tick+(server_tickspeed()*tuning.laser_bounce_delay)/1000.0f)
	{
		do_bounce();
	}

}

void LASER::snap(int snapping_client)
{
	if(distance(game.players[snapping_client].view_pos, pos) > 1000.0f)
		return;

	NETOBJ_LASER *obj = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id, sizeof(NETOBJ_LASER));
	obj->x = (int)pos.x;
	obj->y = (int)pos.y;
	obj->from_x = (int)from.x;
	obj->from_y = (int)from.y;
	obj->start_tick = eval_tick;
}

GAMECONTEXT::GAMECONTEXT()
{
	clear();
}

void GAMECONTEXT::clear()
{
	// reset all players
	for(int i = 0; i < MAX_CLIENTS; i++)
		players[i].init(-1);
}


void GAMECONTEXT::create_damageind(vec2 p, float angle, int amount)
{
	float a = 3 * 3.14159f / 2 + angle;
	//float a = get_angle(dir);
	float s = a-pi/3;
	float e = a+pi/3;
	for(int i = 0; i < amount; i++)
	{
		float f = mix(s, e, float(i+1)/float(amount+2));
		NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)events.create(NETEVENTTYPE_DAMAGEIND, sizeof(NETEVENT_DAMAGEIND));
		if(ev)
		{
			ev->x = (int)p.x;
			ev->y = (int)p.y;
			ev->angle = (int)(f*256.0f);
		}
	}
}

void GAMECONTEXT::create_explosion(vec2 p, int owner, int weapon, bool bnodamage)
{
	// create the event
	NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)events.create(NETEVENTTYPE_EXPLOSION, sizeof(NETEVENT_EXPLOSION));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}

	if (!bnodamage)
	{
		// deal damage
		CHARACTER *ents[64];
		float radius = 128.0f;
		float innerradius = 42.0f;
		int num = game.world.find_entities(p, radius, (ENTITY**)ents, 64, NETOBJTYPE_CHARACTER);
		for(int i = 0; i < num; i++)
		{
			vec2 diff = ents[i]->pos - p;
			vec2 forcedir(0,1);
			float l = length(diff);
			if(l)
				forcedir = normalize(diff);
			l = 1-clamp((l-innerradius)/(radius-innerradius), 0.0f, 1.0f);
			float dmg = 6 * l;
			if((int)dmg)
				ents[i]->take_damage(forcedir*dmg*2, (int)dmg, owner, weapon);
		}
	}
}

/*
void create_smoke(vec2 p)
{
	// create the event
	EV_EXPLOSION *ev = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}*/

void GAMECONTEXT::create_playerspawn(vec2 p)
{
	// create the event
	NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)events.create(NETEVENTTYPE_SPAWN, sizeof(NETEVENT_SPAWN));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
	}
}

void GAMECONTEXT::create_death(vec2 p, int cid)
{
	// create the event
	NETEVENT_DEATH *ev = (NETEVENT_DEATH *)events.create(NETEVENTTYPE_DEATH, sizeof(NETEVENT_DEATH));
	if(ev)
	{
		ev->x = (int)p.x;
		ev->y = (int)p.y;
		ev->cid = cid;
	}
}

void GAMECONTEXT::create_sound(vec2 pos, int sound, int mask)
{
	if (sound < 0)
		return;

	// create a sound
	NETEVENT_SOUNDWORLD *ev = (NETEVENT_SOUNDWORLD *)events.create(NETEVENTTYPE_SOUNDWORLD, sizeof(NETEVENT_SOUNDWORLD), mask);
	if(ev)
	{
		ev->x = (int)pos.x;
		ev->y = (int)pos.y;
		ev->soundid = sound;
	}
}

void GAMECONTEXT::create_sound_global(int sound, int target)
{
	if (sound < 0)
		return;

	NETMSG_SV_SOUNDGLOBAL msg;
	msg.soundid = sound;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(target);
}

// TODO: should be more general
CHARACTER *GAMEWORLD::intersect_character(vec2 pos0, vec2 pos1, float radius, vec2& new_pos, ENTITY *notthis)
{
	// Find other players
	float closest_len = distance(pos0, pos1) * 100.0f;
	vec2 line_dir = normalize(pos1-pos0);
	CHARACTER *closest = 0;

	CHARACTER *p = (CHARACTER *)game.world.find_first(NETOBJTYPE_CHARACTER);
	for(; p; p = (CHARACTER *)p->typenext())
 	{
		if(p == notthis)
			continue;
			
		vec2 intersect_pos = closest_point_on_line(pos0, pos1, p->pos);
		float len = distance(p->pos, intersect_pos);
		if(len < CHARACTER::phys_size+radius)
		{
			if(len < closest_len)
			{
				new_pos = intersect_pos;
				closest_len = len;
				closest = p;
			}
		}
	}
	
	return closest;
}


CHARACTER *GAMEWORLD::closest_character(vec2 pos, float radius, ENTITY *notthis)
{
	// Find other players
	float closest_range = radius*2;
	CHARACTER *closest = 0;
		
	CHARACTER *p = (CHARACTER *)game.world.find_first(NETOBJTYPE_CHARACTER);
	for(; p; p = (CHARACTER *)p->typenext())
 	{
		if(p == notthis)
			continue;
			
		float len = distance(pos, p->pos);
		if(len < CHARACTER::phys_size+radius)
		{
			if(len < closest_range)
			{
				closest_range = len;
				closest = p;
			}
		}
	}
	
	return closest;
}


void GAMECONTEXT::tick()
{
	world.core.tuning = tuning;
	world.tick();

	if(world.paused) // make sure that the game object always updates
		controller->tick();
		
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1)
			players[i].tick();
	}
}

void GAMECONTEXT::snap(int client_id)
{
	world.snap(client_id);
	events.snap(client_id);
	controller->snap(client_id);
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1)
			players[i].snap(client_id);
	}
}

// Server hooks
void mods_client_direct_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id].on_direct_input((NETOBJ_PLAYER_INPUT *)input);
	
	/*
	if(i->fire)
	{
		msg_pack_start(MSG_EXTRA_PROJECTILE, 0);
		msg_pack_end();
		server_send_msg(client_id);
	}*/
}

void mods_client_predicted_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id].on_predicted_input((NETOBJ_PLAYER_INPUT *)input);
	
	/*
	{
		
		on_predicted_input()
		if (memcmp(&game.players[client_id].input, input, sizeof(NETOBJ_PLAYER_INPUT)) != 0)
			game.players[client_id].last_action = server_tick();

		//game.players[client_id].previnput = game.players[client_id].input;
		game.players[client_id].input = *(NETOBJ_PLAYER_INPUT*)input;
		game.players[client_id].num_inputs++;
		
		if(game.players[client_id].input.target_x == 0 && game.players[client_id].input.target_y == 0)
			game.players[client_id].input.target_y = -1;
	}*/
}

// Server hooks
void mods_tick()
{
	game.tick();
}

void mods_snap(int client_id)
{
	game.snap(client_id);
}

void mods_client_enter(int client_id)
{
	//game.world.insert_entity(&game.players[client_id]);
	game.players[client_id].respawn();
	dbg_msg("game", "join player='%d:%s'", client_id, server_clientname(client_id));


	char buf[512];
	str_format(buf, sizeof(buf), "%s entered and joined the %s", server_clientname(client_id), game.controller->get_team_name(game.players[client_id].team));
	game.send_chat(-1, CHAT_ALL, buf); 

	dbg_msg("game", "team_join player='%d:%s' team=%d", client_id, server_clientname(client_id), game.players[client_id].team);
}

void mods_connected(int client_id)
{
	game.players[client_id].init(client_id);
	//game.players[client_id].client_id = client_id;
	
	// Check which team the player should be on
	if(config.sv_tournament_mode)
		game.players[client_id].team = -1;
	else
		game.players[client_id].team = game.controller->get_auto_team(client_id);

	// send motd
	NETMSG_SV_MOTD msg;
	msg.message = config.sv_motd;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(client_id);
}

void mods_client_drop(int client_id)
{
	game.players[client_id].on_disconnect();

}

void mods_message(int msgtype, int client_id)
{
	void *rawmsg = netmsg_secure_unpack(msgtype);
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
			team = game.players[client_id].team;
		else
			team = CHAT_ALL;
		
		if(config.sv_spamprotection && game.players[client_id].last_chat+time_freq() > time_get())
		{
			// consider this as spam
		}
		else
		{
			game.players[client_id].last_chat = time_get();
			game.send_chat(client_id, team, msg->message);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_SETTEAM)
	{
		NETMSG_CL_SETTEAM *msg = (NETMSG_CL_SETTEAM *)rawmsg;

		// Switch team on given client and kill/respawn him
		if(game.controller->can_join_team(msg->team, client_id))
			game.players[client_id].set_team(msg->team);
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
		game.players[client_id].use_custom_color = msg->use_custom_color;
		game.players[client_id].color_body = msg->color_body;
		game.players[client_id].color_feet = msg->color_feet;

		// check for invalid chars
		/*
		unsigned char *p = (unsigned char *)name;
		while (*p)
		{
			if(*p < 32)
				*p = ' ';
			p++;
		}*/

		// copy old name
		char oldname[MAX_NAME_LENGTH];
		str_copy(oldname, server_clientname(client_id), MAX_NAME_LENGTH);
		
		server_setclientname(client_id, msg->name);
		if(msgtype == NETMSGTYPE_CL_CHANGEINFO && strcmp(oldname, server_clientname(client_id)) != 0)
		{
			char chattext[256];
			str_format(chattext, sizeof(chattext), "%s changed name to %s", oldname, server_clientname(client_id));
			game.send_chat(-1, CHAT_ALL, chattext);
		}
		
		// set skin
		str_copy(game.players[client_id].skin_name, msg->skin, sizeof(game.players[client_id].skin_name));
		
		game.controller->on_player_info_change(&game.players[client_id]);
		
		if(msgtype == NETMSGTYPE_CL_STARTINFO)
		{
			// a client that connected!
			
			// send all info to this client
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(game.players[i].client_id != -1)
					game.send_info(i, client_id);
			}

			// send tuning parameters to client
			send_tuning_params(client_id);
			
			//
			NETMSG_SV_READYTOENTER m;
			m.pack(MSGFLAG_VITAL|MSGFLAG_FLUSH);
			server_send_msg(client_id);			
		}
		
		game.send_info(client_id, -1);
	}
	else if (msgtype == NETMSGTYPE_CL_EMOTICON)
	{
		NETMSG_CL_EMOTICON *msg = (NETMSG_CL_EMOTICON *)rawmsg;
		game.send_emoticon(client_id, msg->emoticon);
	}
	else if (msgtype == NETMSGTYPE_CL_KILL)
	{
		//PLAYER *pplayer = get_player(client_id);
		game.players[client_id].kill_character(); //(client_id, -1);
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
	game.send_chat(-1, CHAT_ALL, console_arg_string(result, 0));
}

static void con_set_team(void *result, void *user_data)
{
	int client_id = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS);
	int team = clamp(console_arg_int(result, 1), -1, 1);
	
	dbg_msg("", "%d %d", client_id, team);
	
	if(game.players[client_id].client_id != client_id)
		return;
	
	game.players[client_id].set_team(team);
}

void mods_console_init()
{
	MACRO_REGISTER_COMMAND("tune", "si", con_tune_param, 0);
	MACRO_REGISTER_COMMAND("tune_reset", "", con_tune_reset, 0);
	MACRO_REGISTER_COMMAND("tune_dump", "", con_tune_dump, 0);

	MACRO_REGISTER_COMMAND("restart", "?i", con_restart, 0);
	MACRO_REGISTER_COMMAND("broadcast", "r", con_broadcast, 0);
	MACRO_REGISTER_COMMAND("say", "r", con_say, 0);
	MACRO_REGISTER_COMMAND("set_team", "ii", con_set_team, 0);
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
	if(strcmp(config.sv_gametype, "ctf") == 0)
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
			int index = tiles[y*tmap->width+x].index - ENTITY_OFFSET;
			vec2 pos(x*32.0f+16.0f, y*32.0f+16.0f);
			game.controller->on_entity(index, pos);
		}
	}

	//game.world.insert_entity(game.controller);

	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			mods_connected(MAX_CLIENTS-i-1);
			mods_client_enter(MAX_CLIENTS-i-1);
			if(game.controller->gametype != GAMETYPE_DM)
				game.players[MAX_CLIENTS-i-1].team = i&1;
		}
	}
}

void mods_shutdown()
{
	delete game.controller;
	game.controller = 0;
}

void mods_presnap() {}
void mods_postsnap()
{
	game.events.clear();
}

extern "C" const char *mods_net_version() { return GAME_NETVERSION; }
extern "C" const char *mods_version() { return GAME_VERSION; }
