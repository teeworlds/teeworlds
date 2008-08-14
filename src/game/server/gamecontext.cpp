#include <engine/e_server_interface.h>
#include "gamecontext.hpp"

GAMECONTEXT game;

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



void GAMECONTEXT::tick()
{
	world.core.tuning = tuning;
	world.tick();

	//if(world.paused) // make sure that the game object always updates
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
	controller->snap(client_id);
	events.snap(client_id);
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(players[i].client_id != -1)
			players[i].snap(client_id);
	}
}
