#include <baselib/math.h>
//#include <baselib/keys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <engine/config.h>
#include "../game.h"
#include "mapres_image.h"
#include "mapres_tilemap.h"
#include "data.h"

using namespace baselib;

static data_container *data;

int charids[16] = {2,10,0,4,12,6,14,1,9,15,13,11,7,5,8,3};

static vec2 mouse_pos;
static vec2 local_player_pos;
static obj_player *local_player;

struct client_data
{
	char name[64];
} client_datas[MAX_CLIENTS];

inline float frandom() { return rand()/(float)(RAND_MAX); }

void snd_play_random(int setid, float vol, float pan)
{
	soundset *set = &data->sounds[setid];
	
	if(!set->num_sounds)
		return;
		
	if(set->num_sounds == 1)
	{
		snd_play(set->sounds[0].id, SND_PLAY_ONCE, vol, pan);
		return;
	}
	
	// play a random one
	int id;
	do {
		id = rand() % set->num_sounds;
	} while(id == set->last);
	snd_play(set->sounds[id].id, SND_PLAY_ONCE, vol, pan);
	set->last = id;
}

// sound volume tweak
static const float stereo_separation = 0.01f;
static const float stereo_separation_deadzone = 512.0f;
static const float volume_distance_falloff = 100.0f;
static const float volume_distance_deadzone = 512.0f;
static const float volume_gun = 0.5f;
static const float volume_tee = 0.5f;
static const float volume_hit = 0.5f;
static const float volume_music = 0.8f;

void sound_vol_pan(const vec2& p, float *vol, float *pan)
{
	vec2 player_to_ev = p - local_player_pos;
	*pan = 0.0f;
	*vol = 1.0f;

	if(fabs(player_to_ev.x) > stereo_separation_deadzone)
	{
		*pan = stereo_separation * (player_to_ev.x - sign(player_to_ev.x)*stereo_separation_deadzone);
		if(*pan < -1.0f) *pan = -1.0f;
		if(*pan > 1.0f) *pan = 1.0f;
	}

	float len = length(player_to_ev);
	if(len > volume_distance_deadzone)
	{
		*vol = volume_distance_falloff / (len - volume_distance_deadzone);

		if(*vol < 0.0f) *vol = 0.0f;
		if(*vol > 1.0f) *vol = 1.0f;
	}
}

enum
{
	SPRITE_FLAG_FLIP_Y=1,
};

static float sprite_w_scale;
static float sprite_h_scale;

static void select_sprite(sprite *spr, int flags=0, int sx=0, int sy=0)
{
	int x = spr->x+sx;
	int y = spr->y+sy;
	int w = spr->w;
	int h = spr->h;
	int cx = spr->set->gridx;
	int cy = spr->set->gridy;
	
	float f = sqrtf(h*h + w*w);
	sprite_w_scale = w/f;
	sprite_h_scale = h/f;
	
	if(flags&SPRITE_FLAG_FLIP_Y)
		gfx_quads_setsubset(x/(float)cx,(y+h)/(float)cy,(x+w)/(float)cx,y/(float)cy);
	else
		gfx_quads_setsubset(x/(float)cx,y/(float)cy,(x+w)/(float)cx,(y+h)/(float)cy);
}

static void select_sprite(int id, int flags=0, int sx=0, int sy=0)
{
	if(id < 0 || id > data->num_sprites)
		return;
	select_sprite(&data->sprites[id], flags, sx, sy);
}

static void draw_sprite(float x, float y, float size)
{
	gfx_quads_draw(x, y, size*sprite_w_scale, size*sprite_h_scale);
}

void move_point(vec2 *inout_pos, vec2 *inout_vel, float elasticity)
{
	vec2 pos = *inout_pos;
	vec2 vel = *inout_vel;
	if(col_check_point(pos + vel))
	{
		int affected = 0;
		if(col_check_point(pos.x + vel.x, pos.y))
		{
			inout_vel->x *= -elasticity;
			affected++;
		}

		if(col_check_point(pos.x, pos.y + vel.y))
		{
			inout_vel->y *= -elasticity;
			affected++;
		}
		
		if(affected == 0)
		{
			inout_vel->x *= -elasticity;
			inout_vel->y *= -elasticity;
		}
	}
	else
	{
		*inout_pos = pos + vel;
	}
}

class damage_indicators
{
public:
	int64 lastupdate;
	struct item
	{
		vec2 pos;
		vec2 dir;
		float life;
		float startangle;
	};
	
	enum
	{
		MAX_ITEMS=64,
	};

	damage_indicators()
	{
		lastupdate = 0;
		num_items = 0;
	}
	
	item items[MAX_ITEMS];
	int num_items;
	
	item *create_i()
	{
		if (num_items < MAX_ITEMS)
		{
			item *p = &items[num_items];
			num_items++;
			return p;
		}
		return 0;
	}
	
	void destroy_i(item *i)
	{
		num_items--;
		*i = items[num_items];
	}
	
	void create(vec2 pos, vec2 dir)
	{
		item *i = create_i();
		if (i)
		{
			i->pos = pos;
			i->life = 0.75f;
			i->dir = dir;
			i->startangle = (( (float)rand()/(float)RAND_MAX) - 1.0f) * 2.0f * pi;
		}
	}
	
	void render()
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		for(int i = 0; i < num_items;)
		{
			vec2 pos = mix(items[i].pos+items[i].dir*75.0f, items[i].pos, clamp((items[i].life-0.60f)/0.15f, 0.0f, 1.0f));
			
			items[i].life -= client_frametime();
			if(items[i].life < 0.0f)
				destroy_i(&items[i]);
			else
			{
				gfx_quads_setcolor(1.0f,1.0f,1.0f, items[i].life/0.1f);
				gfx_quads_setrotation(items[i].startangle + items[i].life * 2.0f);
				select_sprite(SPRITE_STAR1);
				draw_sprite(pos.x, pos.y, 48.0f);
				i++;
			}
		}
		gfx_quads_end();
	}
	
};

static damage_indicators damageind;

class particle_system
{
public:
	struct particle
	{
		vec2 pos;
		vec2 vel;
		float life;
		float max_life;
		float size;
		
		float rot;
		float rotspeed;
		
		float gravity;
		float friction;
		int iparticle;
		
		vec4 color;
	};

	enum
	{
		MAX_PARTICLES=1024,
	};
	
	particle particles[MAX_PARTICLES];
	int num_particles;
	
	particle_system()
	{
		num_particles = 0;
	}
	
	void new_particle(vec2 pos, vec2 vel, float life, float size, float gravity, float friction)
	{
		if (num_particles >= MAX_PARTICLES)
			return;

		particles[num_particles].iparticle = rand() % data->num_particles;
		particles[num_particles].pos = pos;
		particles[num_particles].vel = vel;
		particles[num_particles].life = life - (data->particles[particles[num_particles].iparticle].lifemod/100.0f) * life;
		particles[num_particles].size = size;
		particles[num_particles].max_life = life;
		particles[num_particles].gravity = gravity;
		particles[num_particles].friction = friction;
		particles[num_particles].rot = frandom()*pi*2;
		particles[num_particles].rotspeed = frandom() * 10.0f;
		num_particles++;
	}
	
	void update(float time_passed)
	{
		for(int i = 0; i < num_particles; i++)
		{
			particles[i].vel.y += particles[i].gravity*time_passed;
			particles[i].vel *= particles[i].friction;
			vec2 vel = particles[i].vel*time_passed;
			move_point(&particles[i].pos, &vel, 0.1f+0.9f*frandom());
			particles[i].vel = vel* (1.0f/time_passed);
			particles[i].life += time_passed;
			particles[i].rot += time_passed * particles[i].rotspeed;
			
			// check particle death
			if(particles[i].life > particles[i].max_life)
			{
				num_particles--;
				particles[i] = particles[num_particles];
				i--;
			}
		}
	}
	
	void render()
	{
		gfx_blend_additive();
		gfx_texture_set(data->images[IMAGE_PARTICLES].id);
		gfx_quads_begin();
		
		for(int i = 0; i < num_particles; i++)
		{
			int type = particles[i].iparticle;
			select_sprite(data->particles[type].spr);
			float a = 1 - particles[i].life / particles[i].max_life;
			vec2 p = particles[i].pos;
			
			gfx_quads_setrotation(particles[i].rot);
			
			gfx_quads_setcolor(
				data->particles[type].color_r,
				data->particles[type].color_g,
				data->particles[type].color_b,
				pow(a, 0.75f));
				
			gfx_quads_draw(p.x, p.y,particles[i].size,particles[i].size);
		}
		gfx_quads_end();		
		gfx_blend_normal();
	}
};

static particle_system temp_system;
 
 
static bool chat_active = false;
static char chat_input[512];
static unsigned chat_input_len;
static const int chat_max_lines = 10;

struct chatline
{
	int tick;
	char text[512+64];
};

chatline chat_lines[chat_max_lines];
static int chat_current_line = 0;

void chat_reset()
{
	for(int i = 0; i < chat_max_lines; i++)
		chat_lines[i].tick = -1000000;
	chat_current_line = 0;
}

void chat_add_line(int client_id, const char *line)
{
	chat_current_line = (chat_current_line+1)%chat_max_lines;
	chat_lines[chat_current_line].tick = client_tick();
	sprintf(chat_lines[chat_current_line].text, "%s: %s", client_datas[client_id].name, line); // TODO: abit nasty
}
 
void modc_init()
{
	// load the data container
	data = load_data_container("data/client.dat");
	
	// load sounds
	for(int s = 0; s < data->num_sounds; s++)
		for(int i = 0; i < data->sounds[s].num_sounds; i++)
			data->sounds[s].sounds[i].id = snd_load_wav(data->sounds[s].sounds[i].filename);
	
	// load textures
	for(int i = 0; i < data->num_images; i++)
		data->images[i].id = gfx_load_texture(data->images[i].filename);
}

void modc_entergame()
{
	col_init(32);
	img_init();
	tilemap_init();
	chat_reset();
	
	for(int i = 0; i < MAX_CLIENTS; i++)
		client_datas[i].name[0] = 0;
}

void modc_shutdown()
{
}

void modc_newsnapshot()
{
	int num = snap_num_items(SNAP_CURRENT);
	for(int i = 0; i < num; i++)
	{
		snap_item item;
		void *data = snap_get_item(SNAP_CURRENT, i, &item);
		
		if(item.type == EVENT_DAMAGEINDICATION)
		{
			ev_damageind *ev = (ev_damageind *)data;
			damageind.create(vec2(ev->x, ev->y), get_direction(ev->angle));
		}
		else if(item.type == EVENT_EXPLOSION)
		{
			ev_explosion *ev = (ev_explosion *)data;
			vec2 p(ev->x, ev->y);
			
			// center explosion
			temp_system.new_particle(p, vec2(0,0), 0.3f, 96.0f, 0, 0.95f);
			temp_system.new_particle(p, vec2(0,0), 0.3f, 64.0f, 0, 0.95f);
			temp_system.new_particle(p, vec2(0,0), 0.3f, 32.0f, 0, 0.95f);
			temp_system.new_particle(p, vec2(0,0), 0.3f, 16.0f, 0, 0.95f);
			
			for(int i = 0; i < 16; i++)
			{
				vec2 v = normalize(vec2(frandom()-0.5f, frandom()-0.5f))*(128.0f+frandom()*128.0f);
				temp_system.new_particle(p, v, 0.2f+0.25f*frandom(), 16.0f, 0, 0.985f);
			}
			
			for(int i = 0; i < 16; i++)
			{
				vec2 v = normalize(vec2(frandom()-0.5f, frandom()-0.5f))*(256.0f+frandom()*512.0f);
				temp_system.new_particle(p, v, 0.2f+0.25f*frandom(), 16.0f, 128.0f, 0.985f);
			}

			for(int i = 0; i < 64; i++)
			{
				vec2 v = normalize(vec2(frandom()-0.5f, frandom()-0.5f))*(frandom()*256.0f);
				temp_system.new_particle(p, v, 0.2f+0.25f*frandom(), 24.0f, 128.0f, 0.985f);
			}
		}
		else if(item.type == EVENT_SMOKE)
		{
			ev_explosion *ev = (ev_explosion *)data;
			vec2 p(ev->x, ev->y);
			
			// center explosion
			vec2 v = normalize(vec2(frandom()-0.5f, -frandom()))*(32.0f+frandom()*32.0f);
			temp_system.new_particle(p, v, 1.2f, 64.0f, 0, 0.95f);
			v = normalize(vec2(frandom()-0.5f, -frandom()))*(128.0f+frandom()*128.0f);
			temp_system.new_particle(p, v, 1.2f, 32.0f, 0, 0.95f);
			v = normalize(vec2(frandom()-0.5f, -frandom()))*(128.0f+frandom()*128.0f);
			temp_system.new_particle(p, v, 1.2f, 16.0f, 0, 0.95f);
			
			for(int i = 0; i < 8; i++)
			{
				vec2 v = normalize(vec2(frandom()-0.5f, frandom()-0.5f))*(64.0f+frandom()*64.0f);
				temp_system.new_particle(p, v, 0.5f+0.5f*frandom(), 16.0f, 0, 0.985f);
			}
			
			for(int i = 0; i < 8; i++)
			{
				vec2 v = normalize(vec2(frandom()-0.5f, frandom()-0.5f))*(128.0f+frandom()*256.0f);
				temp_system.new_particle(p, v, 0.5f+0.5f*frandom(), 16.0f, 128.0f, 0.985f);
			}
		}
		else if(item.type == EVENT_SOUND)
		{
			ev_sound *ev = (ev_sound *)data;
			vec2 p(ev->x, ev->y);
			int soundid = ev->sound; //(ev->sound & SOUND_MASK);
			//bool bstartloop = (ev->sound & SOUND_LOOPFLAG_STARTLOOP) != 0;
			//bool bstoploop = (ev->sound & SOUND_LOOPFLAG_STOPLOOP) != 0;
			float vol, pan;
			sound_vol_pan(p, &vol, &pan);
			
			if(soundid >= 0 && soundid < NUM_SOUNDS)
			{
				// TODO: we need to control the volume of the diffrent sounds
				// 		depening on the category
				snd_play_random(soundid, vol, pan);
			}
		}
	}
}

static void render_projectile(obj_projectile *prev, obj_projectile *current)
{
	gfx_texture_set(data->images[IMAGE_WEAPONS].id);
	gfx_quads_begin();
	
	select_sprite(data->weapons[current->type%data->num_weapons].sprite_proj);
	vec2 vel(current->vx, current->vy);
	
	// TODO: interpolare angle aswell
	if(length(vel) > 0.00001f)
		gfx_quads_setrotation(get_angle(vel));
	else
		gfx_quads_setrotation(0);
	
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	gfx_quads_draw(pos.x, pos.y,32,32);
	gfx_quads_setrotation(0);
	gfx_quads_end();
}

static void render_powerup(obj_powerup *prev, obj_powerup *current)
{
	gfx_texture_set(data->images[IMAGE_WEAPONS].id);
	gfx_quads_begin();
	float angle = 0.0f;
	float size = 64.0f;
	if (current->type == POWERUP_TYPE_WEAPON)
	{
		angle = -0.25f * pi * 2.0f;
		select_sprite(data->weapons[current->subtype%data->num_weapons].sprite_body);
		size = data->weapons[current->subtype%data->num_weapons].visual_size;
	}
	else
	{
		const int c[] = {
			SPRITE_POWERUP_HEALTH,
			SPRITE_POWERUP_ARMOR,
			SPRITE_POWERUP_WEAPON,
			SPRITE_POWERUP_NINJA,
			SPRITE_POWERUP_TIMEFIELD
			};
		select_sprite(c[current->type]);
	}
	
	gfx_quads_setrotation(angle);
	
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	float offset = pos.y/32.0f + pos.x/32.0f;
	pos.x += cosf(client_localtime()*2.0f+offset)*2.5f;
	pos.y += sinf(client_localtime()*2.0f+offset)*2.5f;
	draw_sprite(pos.x, pos.y, size);
	gfx_quads_end();
}

static void anim_seq_eval(sequence *seq, float time, keyframe *frame)
{
	if(seq->num_frames == 0)
	{
		frame->time = 0;
		frame->x = 0;
		frame->y = 0;
		frame->angle = 0;
	}
	else if(seq->num_frames == 1)
	{
		*frame = seq->frames[0];
	}
	else
	{
		//time = max(0.0f, min(1.0f, time / duration)); // TODO: use clamp
		keyframe *frame1 = 0;
		keyframe *frame2 = 0;
		float blend = 0.0f;

		// TODO: make this smarter.. binary search
		for (int i = 1; i < seq->num_frames; i++)
		{
			if (seq->frames[i-1].time <= time && seq->frames[i].time >= time)
			{
				frame1 = &seq->frames[i-1];
				frame2 = &seq->frames[i];
				blend = (time - frame1->time) / (frame2->time - frame1->time);
				break;
			}
		}		

		if (frame1 && frame2)
		{
			frame->time = time;
			frame->x = mix(frame1->x, frame2->x, blend);
			frame->y = mix(frame1->y, frame2->y, blend);
			frame->angle = mix(frame1->angle, frame2->angle, blend);
		}
	}
}

struct animstate
{
	keyframe body;
	keyframe back_foot;
	keyframe front_foot;
	keyframe attach;
};



static void anim_eval(animation *anim, float time, animstate *state)
{
	anim_seq_eval(&anim->body, time, &state->body);
	anim_seq_eval(&anim->back_foot, time, &state->back_foot);
	anim_seq_eval(&anim->front_foot, time, &state->front_foot);
	anim_seq_eval(&anim->attach, time, &state->attach);
}

static void anim_add_keyframe(keyframe *seq, keyframe *added, float amount)
{
	seq->x += added->x*amount;
	seq->y += added->y*amount;
	seq->angle += added->angle*amount;
}

static void anim_add(animstate *state, animstate *added, float amount)
{
	anim_add_keyframe(&state->body, &added->body, amount);
	anim_add_keyframe(&state->back_foot, &added->back_foot, amount);
	anim_add_keyframe(&state->front_foot, &added->front_foot, amount);
	anim_add_keyframe(&state->attach, &added->attach, amount);
}

static void anim_eval_add(animstate *state, animation *anim, float time, float amount)
{
	animstate add;
	anim_eval(anim, time, &add);
	anim_add(state, &add, amount);
}

static void render_tee(animstate *anim, int skin, vec2 dir, vec2 pos)
{
	vec2 direction =  dir;
	//float angle = info->angle;
	vec2 position = pos;
	
	gfx_texture_set(data->images[IMAGE_CHAR_DEFAULT].id);
	gfx_quads_begin();
	
	// draw foots
	for(int p = 0; p < 2; p++)
	{
		// first pass we draw the outline
		// second pass we draw the filling
		
		int outline = p==0 ? 1 : 0;
		int shift = charids[skin%16];
		
		for(int f = 0; f < 2; f++)
		{
			float basesize = 10.0f;
			if(f == 1)
			{
				// draw body
				select_sprite(outline?SPRITE_TEE_BODY_OUTLINE:SPRITE_TEE_BODY, 0, 0, shift*4);
				gfx_quads_draw(position.x+anim->body.x, position.y+anim->body.y, 4*basesize, 4*basesize);
				
				// draw eyes
				if(p == 1)
				{
					// normal
					select_sprite(SPRITE_TEE_EYE_NORMAL, 0, 0, shift*4);
					gfx_quads_draw(position.x-4+direction.x*4, position.y-8+direction.y*3, basesize, basesize);
					gfx_quads_draw(position.x+4+direction.x*4, position.y-8+direction.y*3, basesize, basesize);
				}
			}

			// draw feet
			select_sprite(outline?SPRITE_TEE_FOOT_OUTLINE:SPRITE_TEE_FOOT, 0, 0, shift*4);
			
			keyframe *foot = f ? &anim->front_foot : &anim->back_foot;
			
			float w = basesize*2.5f;
			float h = basesize*1.425f;
			
			gfx_quads_setrotation(foot->angle);
			gfx_quads_draw(position.x+foot->x, position.y+foot->y, w, h);
		}
	}
	
	gfx_quads_end();	
}

static void render_player(obj_player *prev, obj_player *player)
{
	if(player->health < 0) // dont render dead players
		return;
	
	vec2 direction = get_direction(player->angle);
	float angle = player->angle/256.0f;
	vec2 position = mix(vec2(prev->x, prev->y), vec2(player->x, player->y), client_intratick());
	
	bool stationary = player->vx < 1 && player->vx > -1;
	bool inair = col_check_point(player->x, player->y+16) == 0;
	
	// evaluate animation
	float walk_time = fmod(position.x, 100.0f)/100.0f;
	animstate state;
	anim_eval(&data->animations[ANIM_BASE], 0, &state);

	if(inair)
		anim_eval_add(&state, &data->animations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	else if(stationary)
		anim_eval_add(&state, &data->animations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
	else
		anim_eval_add(&state, &data->animations[ANIM_WALK], walk_time, 1.0f);

	if (player->weapon == WEAPON_HAMMER)
	{
		float a = clamp((client_tick()-player->attacktick+client_intratick())/7.5f, 0.0f, 1.0f);
		anim_eval_add(&state, &data->animations[ANIM_HAMMER_SWING], a, 1.0f);
	}

		
	// draw hook
	if(player->hook_active)
	{
		gfx_texture_set(data->images[IMAGE_WEAPONS].id);
		gfx_quads_begin();
		//gfx_quads_begin();

		vec2 pos = position;
		vec2 hook_pos = mix(vec2(prev->hook_x, prev->hook_y), vec2(player->hook_x, player->hook_y), client_intratick());
		
		float d = distance(pos, hook_pos);
		vec2 dir = normalize(pos-hook_pos);

		gfx_quads_setrotation(get_angle(dir)+pi);
		
		// render head
		select_sprite(SPRITE_HOOK_HEAD);
		gfx_quads_draw(hook_pos.x, hook_pos.y, 24,16);
		
		// render chain
		select_sprite(SPRITE_HOOK_CHAIN);
		for(float f = 24; f < d; f += 24)
		{
			vec2 p = hook_pos + dir*f;
			gfx_quads_draw(p.x, p.y,24,16);
		}
		
		gfx_quads_setrotation(0);
		gfx_quads_end();
	}

	// draw gun
	{
		gfx_texture_set(data->images[IMAGE_WEAPONS].id);
		gfx_quads_begin();
		gfx_quads_setrotation(state.attach.angle*pi*2+angle);

		// normal weapons
		int iw = clamp(player->weapon, 0, NUM_WEAPONS-1);
		select_sprite(data->weapons[iw].sprite_body, direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

		vec2 dir = direction;
		float recoil = 0.0f;
		if (player->weapon == WEAPON_HAMMER)
		{
			// if attack is under way, bash stuffs
			if(direction.x < 0)
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
			else
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
		}
		else
		{
			// TODO: should be an animation
			recoil = 0;
			float a = (client_tick()-player->attacktick+client_intratick())/5.0f;
			if(a < 1)
				recoil = sinf(a*pi);
		}

		vec2 p = position + dir*20.0f - dir*recoil*10.0f;
		draw_sprite(p.x, p.y, data->weapons[iw].visual_size);
		
		// TODO: draw muzzleflare
		gfx_quads_end();
	}
	
	render_tee(&state, player->clientid, direction, position);
	
	/*
	gfx_texture_set(data->images[IMAGE_CHAR_DEFAULT].id);
	gfx_quads_begin();
	
	// draw foots
	for(int p = 0; p < 2; p++)
	{
		// first pass we draw the outline
		// second pass we draw the filling
		
		int outline = p==0 ? 1 : 0;
		int shift = charids[player->clientid%16];
		
		for(int f = 0; f < 2; f++)
		{
			float basesize = 10.0f;
			if(f == 1)
			{
				// draw body
				select_sprite(outline?SPRITE_TEE_BODY_OUTLINE:SPRITE_TEE_BODY, 0, 0, shift*4);
				gfx_quads_draw(position.x+state.body.x, position.y+state.body.y, 4*basesize, 4*basesize);
				
				// draw eyes
				if(p == 1)
				{
					vec2 md = get_direction(player->angle);
					float mouse_dir_x = md.x;
					float mouse_dir_y = md.y;
					
					// normal
					select_sprite(SPRITE_TEE_EYE_NORMAL, 0, 0, shift*4);
					gfx_quads_draw(position.x-4+mouse_dir_x*4, position.y-8+mouse_dir_y*3, basesize, basesize);
					gfx_quads_draw(position.x+4+mouse_dir_x*4, position.y-8+mouse_dir_y*3, basesize, basesize);
				}
			}

			// draw feet
			select_sprite(outline?SPRITE_TEE_FOOT_OUTLINE:SPRITE_TEE_FOOT, 0, 0, shift*4);
			
			keyframe *foot = f ? &state.front_foot : &state.back_foot;
			
			float w = basesize*2.5f;
			float h = basesize*1.425f;
			
			gfx_quads_setrotation(foot->angle);
			gfx_quads_draw(position.x+foot->x, position.y+foot->y, w, h);
		}
	}
	
	gfx_quads_end();
	*/
}



void modc_render()
{	
	if(inp_key_down(input::enter))
	{
		if(chat_active)
		{
			// send message
			msg_pack_start(MSG_SAY, MSGFLAG_VITAL);
			msg_pack_string(chat_input, 512);
			msg_pack_end();
			client_send_msg();
		}
		else
		{
			mem_zero(chat_input, sizeof(chat_input));
			chat_input_len = 0;
		}
		chat_active = !chat_active;
	}
	
	if(chat_active)
	{
		int c = input::last_char(); // TODO: bypasses the engine interface
		int k = input::last_key(); // TODO: bypasses the engine interface
	
		if (c >= 32 && c < 255)
		{
			if (chat_input_len < sizeof(chat_input) - 1)
			{
				chat_input[chat_input_len] = c;
				chat_input[chat_input_len+1] = 0;
				chat_input_len++;
			}
		}

		if(k == input::backspace)
		{
			if(chat_input_len > 0)
			{
				chat_input[chat_input_len-1] = 0;
				chat_input_len--;
			}
		}
		
	}
	
	input::clear_char(); // TODO: bypasses the engine interface
	input::clear_key(); // TODO: bypasses the engine interface
	
	// fetch new input
	{
		int x, y;
		inp_mouse_relative(&x, &y);
		mouse_pos += vec2(x, y);
		float l = length(mouse_pos);
		if(l > 600.0f)
			mouse_pos = normalize(mouse_pos)*600.0f;
	}
	
	// snap input
	{
		player_input input;
		mem_zero(&input, sizeof(input));

		float a = atan((float)mouse_pos.y/(float)mouse_pos.x);
		if(mouse_pos.x < 0)
			a = a+pi;
			
		input.angle = (int)(a*256.0f);
		input.activeweapon = -1;
		
		if(!chat_active)
		{
			input.left = inp_key_pressed(config.key_move_left);
			input.right = inp_key_pressed(config.key_move_right);
			input.jump = inp_key_pressed(config.key_jump);
			input.fire = inp_key_pressed(config.key_fire);
			input.hook = inp_key_pressed(config.key_hook);

			input.blink = inp_key_pressed('S');
			
			// Weapon switching
			input.activeweapon = inp_key_pressed('1') ? 0 : input.activeweapon;
			input.activeweapon = inp_key_pressed('2') ? 1 : input.activeweapon;
			input.activeweapon = inp_key_pressed('3') ? 2 : input.activeweapon;
			input.activeweapon = inp_key_pressed('4') ? 3 : input.activeweapon;
		}

		snap_input(&input, sizeof(input));
	}

	// setup world view
	{
		// 1. fetch local player
		// 2. set him to the center
		int num = snap_num_items(SNAP_CURRENT);
		for(int i = 0; i < num; i++)
		{
			snap_item item;
			void *data = snap_get_item(SNAP_CURRENT, i, &item);
			
			if(item.type == OBJTYPE_PLAYER)
			{
				obj_player *player = (obj_player *)data;
				if(player->local)
				{
					local_player = player;
					local_player_pos = vec2(player->x, player->y);

					void *p = snap_find_item(SNAP_PREV, item.type, item.id);
					if(p)
						local_player_pos = mix(vec2(((obj_player *)p)->x, ((obj_player *)p)->y), local_player_pos, client_intratick());
					break;
				}
			}
		}
	}

	// pseudo format
	float zoom = 3.0f;
	
	float width = 400*zoom;
	float height = 300*zoom;
	float screen_x = 0;
	float screen_y = 0;
	
	// center at char but can be moved when mouse is far away
	float offx = 0, offy = 0;
	int deadzone = 300;
	if(mouse_pos.x > deadzone) offx = mouse_pos.x-deadzone;
	if(mouse_pos.x <-deadzone) offx = mouse_pos.x+deadzone;
	if(mouse_pos.y > deadzone) offy = mouse_pos.y-deadzone;
	if(mouse_pos.y <-deadzone) offy = mouse_pos.y+deadzone;
	offx = offx*2/3;
	offy = offy*2/3;
		
	screen_x = local_player_pos.x+offx;
	screen_y = local_player_pos.y+offy;
	
	gfx_mapscreen(screen_x-width/2, screen_y-height/2, screen_x+width/2, screen_y+height/2);
	
	// draw background
	gfx_clear(0.65f,0.78f,0.9f);

	// draw the sun	
	{
		vec2 pos(local_player_pos.x*0.5f, local_player_pos.y*0.5f);

		gfx_texture_set(-1);
		gfx_blend_additive();
		gfx_quads_begin();
		const int rays = 10;
		gfx_quads_setcolor(1.0f,1.0f,1.0f,0.025f);
		for(int r = 0; r < rays; r++)
		{
			float a = r/(float)rays + client_localtime()*0.05f;
			float size = (1.0f/(float)rays)*0.25f;
			vec2 dir0(sinf((a-size)*pi*2.0f), cosf((a-size)*pi*2.0f));
			vec2 dir1(sinf((a+size)*pi*2.0f), cosf((a+size)*pi*2.0f));
			
			gfx_quads_setcolorvertex(0, 1.0f,1.0f,1.0f,0.025f);
			gfx_quads_setcolorvertex(1, 1.0f,1.0f,1.0f,0.025f);
			gfx_quads_setcolorvertex(2, 1.0f,1.0f,1.0f,0.0f);
			gfx_quads_setcolorvertex(3, 1.0f,1.0f,1.0f,0.0f);
			const float range = 1000.0f;
			gfx_quads_draw_freeform(
				pos.x+dir0.x, pos.y+dir0.y,
				pos.x+dir1.x, pos.y+dir1.y,
				pos.x+dir0.x*range, pos.y+dir0.y*range,
				pos.x+dir1.x*range, pos.y+dir1.y*range);
		}
		gfx_quads_end();
		gfx_blend_normal();
		
		gfx_texture_set(data->images[IMAGE_SUN].id);
		gfx_quads_begin();
		gfx_quads_draw(pos.x, pos.y, 256, 256);
		gfx_quads_end();
	}
	
	// render map
	tilemap_render(32.0f, 0);
	// render items
	int num = snap_num_items(SNAP_CURRENT);
	for(int i = 0; i < num; i++)
	{
		snap_item item;
		void *data = snap_get_item(SNAP_CURRENT, i, &item);
		
		if(item.type == OBJTYPE_PLAYER)
		{
			void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_player((obj_player *)prev, (obj_player *)data);
		}
		else if(item.type == OBJTYPE_PROJECTILE)
		{
			void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_projectile((obj_projectile *)prev, (obj_projectile *)data);
		}
		else if(item.type == OBJTYPE_POWERUP)
		{
			void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_powerup((obj_powerup*)prev, (obj_powerup *)data);
		}
	}

	// render particles
	temp_system.update(client_frametime());
	temp_system.render();

	tilemap_render(32.0f, 1);
	
	// render damage indications
	damageind.render();
	
	if(local_player)
	{
		gfx_texture_set(data->images[IMAGE_WEAPONS].id);
		gfx_quads_begin();
		
		// render cursor
		select_sprite(data->weapons[local_player->weapon%data->num_weapons].sprite_cursor);
		float cursorsize = 64;
		draw_sprite(local_player_pos.x+mouse_pos.x, local_player_pos.y+mouse_pos.y, cursorsize);

		// render ammo count
		// render gui stuff
		gfx_quads_end();
		gfx_quads_begin();
		gfx_mapscreen(0,0,400,300);
		select_sprite(data->weapons[local_player->weapon%data->num_weapons].sprite_proj);
		for (int i = 0; i < local_player->ammocount; i++)
			gfx_quads_drawTL(10+i*12,34,10,10);
		gfx_quads_end();

		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		int h = 0;
		
		// render health
		select_sprite(SPRITE_HEALTH_FULL);
		for(; h < local_player->health; h++)
			gfx_quads_drawTL(10+h*12,10,10,10);
		
		select_sprite(SPRITE_HEALTH_EMPTY);
		for(; h < 10; h++)
			gfx_quads_drawTL(10+h*12,10,10,10);

		// render armor meter
		h = 0;
		select_sprite(SPRITE_ARMOR_FULL);
		for(; h < local_player->armor; h++)
			gfx_quads_drawTL(10+h*12,22,10,10);
		
		select_sprite(SPRITE_ARMOR_EMPTY);
		for(; h < 10; h++)
			gfx_quads_drawTL(10+h*12,22,10,10);
		gfx_quads_end();
	}
	
	// render gui stuff
	gfx_mapscreen(0,0,400,300);
	
	{
		float x = 10.0f;
		float y = 300.0f-50.0f;
		float starty = -1;
		if(chat_active)
		{
			
			gfx_texture_set(-1); // TODO: remove when the font looks better
			gfx_quads_begin();
			gfx_quads_setcolor(0,0,0,0.4f);
			gfx_quads_drawTL(x-2, y+1, 300, 8);
			gfx_quads_end();
			
			// render chat input
			char buf[sizeof(chat_input)+16];
			sprintf(buf, "Chat: %s_", chat_input);
			gfx_pretty_text(x, y, 10, buf);
			starty = y;
		}
		
		y -= 10;
		
		int i;
		for(i = 0; i < chat_max_lines; i++)
		{
			int r = ((chat_current_line-i)+chat_max_lines)%chat_max_lines;
			if(client_tick() > chat_lines[r].tick+50*15)
				break;

			gfx_texture_set(-1); // TODO: remove when the font looks better
			gfx_quads_begin();
			gfx_quads_setcolor(0,0,0,0.4f);
			gfx_quads_drawTL(x-2, y+1, gfx_pretty_text_width(10, chat_lines[r].text)+3, 8);
			gfx_quads_end();

			gfx_pretty_text(x, y, 10, chat_lines[r].text);
			y -= 8;
		}
	}
	
	// render score board
	if(inp_key_pressed(baselib::input::tab))
	{
		gfx_mapscreen(0, 0, width, height);

		float x = 50.0f;
		float y = 150.0f;

		gfx_blend_normal();
		
		gfx_texture_set(-1);
		gfx_quads_begin();
		gfx_quads_setcolor(0,0,0,0.5f);
		gfx_quads_drawTL(x-10.f, y-10.f, 400.0f, 600.0f);
		gfx_quads_end();
		
		//gfx_texture_set(current_font->font_texture);
		gfx_pretty_text(x, y, 64, "Score Board");
		y += 100.0f;
		
		//gfx_texture_set(-1);
		//gfx_quads_text(10, 50, 8, "Score Board");
		animstate state;
		anim_eval(&data->animations[ANIM_BASE], 0, &state);
		anim_eval_add(&state, &data->animations[ANIM_IDLE], 0, 1.0f);

		int num = snap_num_items(SNAP_CURRENT);
		for(int i = 0; i < num; i++)
		{
			snap_item item;
			void *data = snap_get_item(SNAP_CURRENT, i, &item);
			
			if(item.type == OBJTYPE_PLAYER)
			{
				obj_player *player = (obj_player *)data;
				if(player)
				{
					char buf[128];
					sprintf(buf, "%4d", player->score);
					gfx_pretty_text(x+60-gfx_pretty_text_width(48,buf), y, 48, buf);
					gfx_pretty_text(x+128, y, 48, client_datas[player->clientid].name);

					render_tee(&state, player->clientid, vec2(1,0), vec2(x+90, y+24));
					y += 58.0f;
				}
			}
		}
	}
}

void modc_message(int msg)
{
	if(msg == MSG_CHAT)
	{
		int cid = msg_unpack_int();
		const char *message = msg_unpack_string();
		dbg_msg("message", "chat cid=%d msg='%s'", cid, message);
		chat_add_line(cid, message);
	}
	else if(msg == MSG_SETNAME)
	{
		int cid = msg_unpack_int();
		const char *name = msg_unpack_string();
		strncpy(client_datas[cid].name, name, 64);
	}
}
