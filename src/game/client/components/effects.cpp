#include <engine/e_client_interface.h>
//#include <gc_client.hpp>
#include <game/generated/gc_data.hpp>

#include <game/client/components/particles.hpp>
#include <game/client/components/skins.hpp>
#include <game/client/components/flow.hpp>
#include <game/client/components/damageind.hpp>
#include <game/client/components/sounds.hpp>
#include <game/client/gameclient.hpp>

#include "effects.hpp"

inline vec2 random_dir() { return normalize(vec2(frandom()-0.5f, frandom()-0.5f)); }

EFFECTS::EFFECTS()
{
	add_50hz = false;
	add_100hz = false;
}

void EFFECTS::air_jump(vec2 pos)
{
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_AIRJUMP;
	p.pos = pos + vec2(-6.0f, 16.0f);
	p.vel = vec2(0, -200);
	p.life_span = 0.5f;
	p.start_size = 48.0f;
	p.end_size = 0;
	p.rot = frandom()*pi*2;
	p.rotspeed = pi*2;
	p.gravity = 500;
	p.friction = 0.7f;
	p.flow_affected = 0.0f;
	gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);

	p.pos = pos + vec2(6.0f, 16.0f);
	gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);
	
	gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, pos);
}

void EFFECTS::damage_indicator(vec2 pos, vec2 dir)
{
	gameclient.damageind->create(pos, dir);
}

void EFFECTS::powerupshine(vec2 pos, vec2 size)
{
	if(!add_50hz)
		return;
		
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_SLICE;
	p.pos = pos + vec2((frandom()-0.5f)*size.x, (frandom()-0.5f)*size.y);
	p.vel = vec2(0, 0);
	p.life_span = 0.5f;
	p.start_size = 16.0f;
	p.end_size = 0;
	p.rot = frandom()*pi*2;
	p.rotspeed = pi*2;
	p.gravity = 500;
	p.friction = 0.9f;
	p.flow_affected = 0.0f;
	gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);
}

void EFFECTS::smoketrail(vec2 pos, vec2 vel)
{
	if(!add_50hz)
		return;
		
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_SMOKE;
	p.pos = pos;
	p.vel = vel + random_dir()*50.0f;
	p.life_span = 0.5f + frandom()*0.5f;
	p.start_size = 12.0f + frandom()*8;
	p.end_size = 0;
	p.friction = 0.7;
	p.gravity = frandom()*-500.0f;
	gameclient.particles->add(PARTICLES::GROUP_PROJECTILE_TRAIL, &p);
}


void EFFECTS::skidtrail(vec2 pos, vec2 vel)
{
	if(!add_100hz)
		return;
	
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_SMOKE;
	p.pos = pos;
	p.vel = vel + random_dir()*50.0f;
	p.life_span = 0.5f + frandom()*0.5f;
	p.start_size = 24.0f + frandom()*12;
	p.end_size = 0;
	p.friction = 0.7f;
	p.gravity = frandom()*-500.0f;
	p.color = vec4(0.75f,0.75f,0.75f,1.0f);
	gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);	
}

void EFFECTS::bullettrail(vec2 pos)
{
	if(!add_100hz)
		return;
		
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_BALL;
	p.pos = pos;
	p.life_span = 0.25f + frandom()*0.25f;
	p.start_size = 8.0f;
	p.end_size = 0;
	p.friction = 0.7f;
	gameclient.particles->add(PARTICLES::GROUP_PROJECTILE_TRAIL, &p);
}

void EFFECTS::playerspawn(vec2 pos)
{
	for(int i = 0; i < 32; i++)
	{
		PARTICLE p;
		p.set_default();
		p.spr = SPRITE_PART_SHELL;
		p.pos = pos;
		p.vel = random_dir() * (pow(frandom(), 3)*600.0f);
		p.life_span = 0.3f + frandom()*0.3f;
		p.start_size = 64.0f + frandom()*32;
		p.end_size = 0;
		p.rot = frandom()*pi*2;
		p.rotspeed = frandom();
		p.gravity = frandom()*-400.0f;
		p.friction = 0.7f;
		p.color = vec4(0xb5/255.0f, 0x50/255.0f, 0xcb/255.0f, 1.0f);
		gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);
		
	}
}

void EFFECTS::playerdeath(vec2 pos, int cid)
{
	vec3 blood_color(1.0f,1.0f,1.0f);

	if(cid >= 0)	
	{
		const SKINS::SKIN *s = gameclient.skins->get(gameclient.clients[cid].skin_id);
		if(s)
			blood_color = s->blood_color;
	}
	
	for(int i = 0; i < 64; i++)
	{
		PARTICLE p;
		p.set_default();
		p.spr = SPRITE_PART_SPLAT01 + (rand()%3);
		p.pos = pos;
		p.vel = random_dir() * ((frandom()+0.1f)*900.0f);
		p.life_span = 0.3f + frandom()*0.3f;
		p.start_size = 24.0f + frandom()*16;
		p.end_size = 0;
		p.rot = frandom()*pi*2;
		p.rotspeed = (frandom()-0.5f) * pi;
		p.gravity = 800.0f;
		p.friction = 0.8f;
		vec3 c = blood_color * (0.75f + frandom()*0.25f);
		p.color = vec4(c.r, c.g, c.b, 0.75f);
		gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);
	}
}


void EFFECTS::explosion(vec2 pos)
{
	// add to flow
	for(int y = -8; y <= 8; y++)
		for(int x = -8; x <= 8; x++)
		{
			if(x == 0 && y == 0)
				continue;
			
			float a = 1 - (length(vec2(x,y)) / length(vec2(8,8)));
			gameclient.flow->add(pos+vec2(x,y)*16, normalize(vec2(x,y))*5000.0f*a, 10.0f);
		}
		
	// add the explosion
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_EXPL01;
	p.pos = pos;
	p.life_span = 0.4f;
	p.start_size = 150.0f;
	p.end_size = 0;
	p.rot = frandom()*pi*2;
	gameclient.particles->add(PARTICLES::GROUP_EXPLOSIONS, &p);
	
	// add the smoke
	for(int i = 0; i < 24; i++)
	{
		PARTICLE p;
		p.set_default();
		p.spr = SPRITE_PART_SMOKE;
		p.pos = pos;
		p.vel = random_dir() * ((1.0f + frandom()*0.2f) * 1000.0f);
		p.life_span = 0.5f + frandom()*0.4f;
		p.start_size = 32.0f + frandom()*8;
		p.end_size = 0;
		p.gravity = frandom()*-800.0f;
		p.friction = 0.4f;
		p.color = mix(vec4(0.75f,0.75f,0.75f,1.0f), vec4(0.5f,0.5f,0.5f,1.0f), frandom());
		gameclient.particles->add(PARTICLES::GROUP_GENERAL, &p);
	}
}


void EFFECTS::hammerhit(vec2 pos)
{
	// add the explosion
	PARTICLE p;
	p.set_default();
	p.spr = SPRITE_PART_EXPL01;
	p.pos = pos;
	p.life_span = 0.4f;
	p.start_size = 150.0f;
	p.end_size = 0;
	p.rot = frandom()*pi*2;
	gameclient.particles->add(PARTICLES::GROUP_EXPLOSIONS, &p);	
	gameclient.sounds->play(SOUNDS::CHN_WORLD, SOUND_HAMMER_HIT, 1.0f, pos);
}

void EFFECTS::on_render()
{
	static int64 last_update_100hz = 0;
	static int64 last_update_50hz = 0;

	if(time_get()-last_update_100hz > time_freq()/100)
	{
		add_100hz = true;
		last_update_100hz = time_get();
	}
	else
		add_100hz = false;

	if(time_get()-last_update_50hz > time_freq()/100)
	{
		add_50hz = true;
		last_update_50hz = time_get();
	}
	else
		add_50hz = false;
		
	if(add_50hz)
		gameclient.flow->update();
}
