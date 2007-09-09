#include <game/math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
	#include <engine/client/ui.h>
	#include <engine/config.h>
};

#include "../game.h"
#include "../version.h"
#include "mapres_image.h"
#include "mapres_tilemap.h"
#include "data.h"
#include "menu.h"

data_container *data = 0x0;

static int charids[16] = {2,10,0,4,12,6,9,1,3,15,13,11,7,5,8,14};

int gametype = GAMETYPE_DM;
static int skinseed = 0;

static int music_menu = -1;
static int music_menu_id = -1;

static bool chat_active = false;
static bool menu_active = false;
static bool emoticon_selector_active = false;

static vec2 mouse_pos;
static vec2 local_player_pos;
static obj_player *local_player;

struct client_data
{
	char name[64];
	int team;
	int emoticon;
	int emoticon_start;
	player_core predicted;
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
static const float stereo_separation = 0.001f;
static const float stereo_separation_deadzone = 200.0f;
static const float volume_distance_falloff = 200.0f;
static const float volume_distance_deadzone = 320.0f;
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

void select_sprite(int id, int flags=0, int sx=0, int sy=0)
{
	if(id < 0 || id > data->num_sprites)
		return;
	select_sprite(&data->sprites[id], flags, sx, sy);
}

void draw_sprite(float x, float y, float size)
{
	gfx_quads_draw(x, y, size*sprite_w_scale, size*sprite_h_scale);
}

/*
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
}*/

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
			i->dir = dir*-1;
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
			move_point(&particles[i].pos, &vel, 0.1f+0.9f*frandom(), NULL);
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
		gfx_texture_set(data->images[IMAGE_GAME].id);
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

class projectile_particles
{
public:
	enum
	{
		LISTSIZE = 1000,
	};
	// meh, just use size % 
	int lastadd[LISTSIZE];
	projectile_particles()
	{
		reset();
	}
	
	void reset()
	{
		for (int i = 0; i < LISTSIZE; i++)
			lastadd[i] = -1000;
	}

	void addparticle(int projectiletype, int projectileid, vec2 pos, vec2 vel)
	{
		int particlespersecond = data->projectileinfo[projectiletype].particlespersecond;
		int lastaddtick = lastadd[projectileid % LISTSIZE];
		if ((client_tick() - lastaddtick) > (client_tickspeed() / particlespersecond))
		{
			lastadd[projectileid % LISTSIZE] = client_tick();
			float life = data->projectileinfo[projectiletype].particlelife;
			float size = data->projectileinfo[projectiletype].particlesize;
			vec2 v = vel * 0.2f + normalize(vec2(frandom()-0.5f, -frandom()))*(32.0f+frandom()*32.0f);
			
			// add the particle (from projectiletype later on, but meh...)
			temp_system.new_particle(pos, v, life, size, 0, 0.95f);
		}
	}
};
static projectile_particles proj_particles;
 
static char chat_input[512];
static unsigned chat_input_len;
static const int chat_max_lines = 10;

struct chatline
{
	int tick;
	int client_id;
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
	chat_lines[chat_current_line].client_id = client_id;

	if(client_id == -1) // server message
		sprintf(chat_lines[chat_current_line].text, "*** %s", line);
	else
		sprintf(chat_lines[chat_current_line].text, "%s: %s", client_datas[client_id].name, line); // TODO: abit nasty
}

struct killmsg
{
	int weapon;
	int victim;
	int killer;
	int tick;
};

static const int killmsg_max = 5;
killmsg killmsgs[killmsg_max];
static int killmsg_current = 0;

extern unsigned char internal_data[];


extern void draw_round_rect(float x, float y, float w, float h, float r);
extern int render_popup(const char *caption, const char *text, const char *button_text);

static void render_loading(float percent)
{
	gfx_clear(0.65f,0.78f,0.9f);
	gfx_mapscreen(0,0,800.0f,600.0f);

	float tw;

	float w = 700;
	float h = 200;
	float x = 800/2-w/2;
	float y = 600/2-h/2;

	gfx_blend_normal();
	
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_quads_setcolor(0,0,0,0.50f);
	draw_round_rect(x, y, w, h, 40.0f);
	gfx_quads_end();

	const char *caption = "Loading";
	
	tw = gfx_pretty_text_width(48.0f, caption, -1);
	ui_do_label(x+w/2-tw/2, y+20, caption, 48.0f);

	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_quads_setcolor(1,1,1,1.0f);
	draw_round_rect(x+40, y+h-75, (w-80)*percent, 25, 5.0f);
	gfx_quads_end();

	gfx_swap();
}

extern "C" void modc_init()
{
	// load the data container
	data = load_data_from_memory(internal_data);

	// TODO: should be removed
	music_menu = snd_load_wav("data/audio/music_menu.wav");

	float total = data->num_sounds+data->num_images;
	float current = 0;

	// load sounds
	for(int s = 0; s < data->num_sounds; s++)
	{
		render_loading(current/total);
		for(int i = 0; i < data->sounds[s].num_sounds; i++)
		{
			int id;
			if (strcmp(data->sounds[s].sounds[i].filename + strlen(data->sounds[s].sounds[i].filename) - 3, ".wv") == 0)
				id = snd_load_wv(data->sounds[s].sounds[i].filename);
			else
				id = snd_load_wav(data->sounds[s].sounds[i].filename);

			data->sounds[s].sounds[i].id = id;
		}
		
		current++;
	}
	
	// load textures
	for(int i = 0; i < data->num_images; i++)
	{
		render_loading(current/total);
		data->images[i].id = gfx_load_texture(data->images[i].filename);
		current++;
	}
}

extern "C" void modc_entergame()
{
	col_init(32);
	img_init();
	tilemap_init();
	chat_reset();
	
	proj_particles.reset();
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		client_datas[i].name[0] = 0;
		client_datas[i].team = 0;
		client_datas[i].emoticon = 0;
		client_datas[i].emoticon_start = -1;
	}
		
	for(int i = 0; i < killmsg_max; i++)
		killmsgs[i].tick = -100000;
}

extern "C" void modc_shutdown()
{
}

static bool must_process_events = false;

static void process_events(int s)
{
	int num = snap_num_items(s);
	for(int i = 0; i < num; i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(s, i, &item);
		
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
		else if(item.type == EVENT_SPAWN)
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
		else if(item.type == EVENT_DEATH)
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
	
	must_process_events = false;
}

static player_core predicted_prev_player;
static player_core predicted_player;

extern "C" void modc_predict()
{
	// repredict player
	{
		world_core world;
		int local_cid = -1;
		
		// search for players
		for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
		{
			SNAP_ITEM item;
			const void *data = snap_get_item(SNAP_CURRENT, i, &item);
			
			if(item.type == OBJTYPE_PLAYER)
			{
				const obj_player *player = (const obj_player *)data;
				client_datas[player->clientid].predicted.world = &world;
				world.players[player->clientid] = &client_datas[player->clientid].predicted;
				
				client_datas[player->clientid].predicted.read(player);
				if(player->local)
					local_cid = player->clientid;
			}
		}
		
		// predict
		for(int tick = client_tick(); tick <= client_predtick(); tick++)
		{
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
		}
		
		// get the data from the local player
		if(local_cid != -1)
		{
			predicted_prev_player = predicted_player;
			predicted_player = *world.players[local_cid];
		}
	}
}

extern "C" void modc_newsnapshot()
{
	if(must_process_events)
		process_events(SNAP_PREV);
	must_process_events = true;
	
	if(config.stress)
	{
		if((client_tick()%250) == 0)
		{
			msg_pack_start(MSG_SAY, MSGFLAG_VITAL);
			msg_pack_string("galenskap!!!!", 512);
			msg_pack_end();
			client_send_msg();
		}
	}
}

void send_changename_request(const char *name)
{
	msg_pack_start(MSG_CHANGENAME, MSGFLAG_VITAL);
	msg_pack_string(name, 64);
	msg_pack_end();
	client_send_msg();
}

void send_emoticon(int emoticon)
{
	msg_pack_start(MSG_EMOTICON, MSGFLAG_VITAL);
	msg_pack_int(emoticon);
	msg_pack_end();
	client_send_msg();
}

static void render_projectile(const obj_projectile *prev, const obj_projectile *current, int itemid)
{
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	
	select_sprite(data->weapons[current->type%data->num_weapons].sprite_proj);
	vec2 vel = mix(vec2(prev->vx, prev->vy), vec2(current->vx, current->vy), client_intratick());
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	
	// add particle for this projectile
	proj_particles.addparticle(current->type, itemid, pos, vel);
	
	if(length(vel) > 0.00001f)
		gfx_quads_setrotation(get_angle(vel));
	else
		gfx_quads_setrotation(0);
	
	// TODO: do this, but nice
	//temp_system.new_particle(pos, vec2(0,0), 0.3f, 14.0f, 0, 0.95f);

	gfx_quads_draw(pos.x, pos.y,32,32);
	gfx_quads_setrotation(0);
	gfx_quads_end();
}

static void render_powerup(const obj_powerup *prev, const obj_powerup *current)
{
	gfx_texture_set(data->images[IMAGE_GAME].id);
	gfx_quads_begin();
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	float angle = 0.0f;
	float size = 64.0f;
	if (current->type == POWERUP_WEAPON)
	{
		angle = 0; //-pi/6;//-0.25f * pi * 2.0f;
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
		
		if(c[current->type] == SPRITE_POWERUP_NINJA)
		{
			proj_particles.addparticle(0, 0,
				pos+vec2((frandom()-0.5f)*80.0f, (frandom()-0.5f)*20.0f),
				vec2((frandom()-0.5f)*10.0f, (frandom()-0.5f)*10.0f));
			size *= 2.0f;
			pos.x += 10.0f;
		}
	}
	
	gfx_quads_setrotation(angle);
	
	float offset = pos.y/32.0f + pos.x/32.0f;
	pos.x += cosf(client_localtime()*2.0f+offset)*2.5f;
	pos.y += sinf(client_localtime()*2.0f+offset)*2.5f;
	draw_sprite(pos.x, pos.y, size);
	gfx_quads_end();
}

static void render_flag(const obj_flag *prev, const obj_flag *current)
{
	float angle = 0.0f;
	float size = 64.0f;

    gfx_blend_normal();
    gfx_texture_set(-1);
    gfx_quads_begin();
	
	gfx_quads_setrotation(angle);
	
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), client_intratick());
	float offset = pos.y/32.0f + pos.x/32.0f;
	pos.x += cosf(client_localtime()*2.0f+offset)*2.5f;
	pos.y += sinf(client_localtime()*2.0f+offset)*2.5f;

    gfx_quads_setcolor(current->team ? 1 : 0,0,current->team ? 0 : 1,1);
	gfx_quads_setsubset(
		0, // startx
		0, // starty
		1, // endx
		1); // endy								
    gfx_quads_draw(pos.x,pos.y,size,size);
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

static void render_hand(int skin, vec2 center_pos, vec2 dir, float angle_offset, vec2 post_rot_offset)
{
	// for drawing hand
	int shift = charids[skin%16];
	float basesize = 10.0f;
	//dir = normalize(hook_pos-pos);

	vec2 hand_pos = center_pos + dir;
	float angle = get_angle(dir);
	if (dir.x < 0)
		angle -= angle_offset;
	else
		angle += angle_offset;

	vec2 dirx = dir;
	vec2 diry(-dir.y,dir.x);

	if (dir.x < 0)
		diry = -diry;

	hand_pos += dirx * post_rot_offset.x;
	hand_pos += diry * post_rot_offset.y;

	gfx_texture_set(data->images[IMAGE_CHAR_DEFAULT].id);
	gfx_quads_begin();

	// two passes
	for (int i = 0; i < 2; i++)
	{
		bool outline = i == 0;

		select_sprite(outline?SPRITE_TEE_HAND_OUTLINE:SPRITE_TEE_HAND, 0, 0, shift*4);
		gfx_quads_setrotation(angle);
		gfx_quads_draw(hand_pos.x, hand_pos.y, 2*basesize, 2*basesize);
	}

	gfx_quads_setrotation(0);
	gfx_quads_end();
}

static void render_tee(animstate *anim, int skin, int emote, vec2 dir, vec2 pos)
{
	vec2 direction =  dir;
	vec2 position = pos;
	
	gfx_texture_set(data->images[IMAGE_CHAR_DEFAULT].id);
	gfx_quads_begin();
	
	// draw foots
	for(int p = 0; p < 2; p++)
	{
		// first pass we draw the outline
		// second pass we draw the filling
		int outline = p==0 ? 1 : 0;
		int shift = skin;
		
		for(int f = 0; f < 2; f++)
		{
			float basesize = 10.0f;
			if(f == 1)
			{
				gfx_quads_setrotation(anim->body.angle*pi*2);
				// draw body
				select_sprite(outline?SPRITE_TEE_BODY_OUTLINE:SPRITE_TEE_BODY, 0, 0, shift*4);
				gfx_quads_draw(position.x+anim->body.x, position.y+anim->body.y, 4*basesize, 4*basesize);
				
				// draw eyes
				if(p == 1)
				{
					switch (emote)
					{
						case EMOTE_PAIN:
							select_sprite(SPRITE_TEE_EYE_PAIN, 0, 0, shift*4);
							break;
						case EMOTE_HAPPY:
							select_sprite(SPRITE_TEE_EYE_HAPPY, 0, 0, shift*4);
							break;
						case EMOTE_SURPRISE:
							select_sprite(SPRITE_TEE_EYE_SURPRISE, 0, 0, shift*4);
							break;
						case EMOTE_ANGRY:
							select_sprite(SPRITE_TEE_EYE_ANGRY, 0, 0, shift*4);
							break;
						default:
							select_sprite(SPRITE_TEE_EYE_NORMAL, 0, 0, shift*4);
							break;
					}
					int h = emote == EMOTE_BLINK ? (int)(basesize/3) : (int)(basesize);
					gfx_quads_draw(position.x-4+direction.x*4, position.y-8+direction.y*3, basesize, h);
					gfx_quads_draw(position.x+4+direction.x*4, position.y-8+direction.y*3, -basesize, h);
				}
			}

			// draw feet
			select_sprite(outline?SPRITE_TEE_FOOT_OUTLINE:SPRITE_TEE_FOOT, 0, 0, shift*4);
			
			keyframe *foot = f ? &anim->front_foot : &anim->back_foot;
			
			float w = basesize*2.5f;
			float h = basesize*1.425f;
			
			gfx_quads_setrotation(foot->angle*pi*2);
			gfx_quads_draw(position.x+foot->x, position.y+foot->y, w, h);
		}
	}
	
	gfx_quads_end();	
}

void draw_circle(float x, float y, float r, int segments)
{
	float f_segments = (float)segments;
	for(int i = 0; i < segments; i+=2)
	{
		float a1 = i/f_segments * 2*pi;
		float a2 = (i+1)/f_segments * 2*pi;
		float a3 = (i+2)/f_segments * 2*pi;
		float ca1 = cosf(a1);
		float ca2 = cosf(a2);
		float ca3 = cosf(a3);
		float sa1 = sinf(a1);
		float sa2 = sinf(a2);
		float sa3 = sinf(a3);
		
		gfx_quads_draw_freeform(
			x, y,
			x+ca1*r, y+sa1*r,
			x+ca3*r, y+sa3*r,
			x+ca2*r, y+sa2*r);
	}
}

void draw_round_rect(float x, float y, float w, float h, float r)
{
	int num = 8;
	for(int i = 0; i < num; i+=2)
	{
		float a1 = i/(float)num * pi/2;
		float a2 = (i+1)/(float)num * pi/2;
		float a3 = (i+2)/(float)num * pi/2;
		float ca1 = cosf(a1);
		float ca2 = cosf(a2);
		float ca3 = cosf(a3);
		float sa1 = sinf(a1);
		float sa2 = sinf(a2);
		float sa3 = sinf(a3);
		
		gfx_quads_draw_freeform(
			x+r, y+r,
			x+(1-ca1)*r, y+(1-sa1)*r,
			x+(1-ca3)*r, y+(1-sa3)*r,
			x+(1-ca2)*r, y+(1-sa2)*r);

		gfx_quads_draw_freeform(
			x+w-r, y+r,
			x+w-r+ca1*r, y+(1-sa1)*r,
			x+w-r+ca3*r, y+(1-sa3)*r,
			x+w-r+ca2*r, y+(1-sa2)*r);

		gfx_quads_draw_freeform(
			x+r, y+h-r,
			x+(1-ca1)*r, y+h-r+sa1*r,
			x+(1-ca3)*r, y+h-r+sa3*r,
			x+(1-ca2)*r, y+h-r+sa2*r);

		gfx_quads_draw_freeform(
			x+w-r, y+h-r,
			x+w-r+ca1*r, y+h-r+sa1*r,
			x+w-r+ca3*r, y+h-r+sa3*r,
			x+w-r+ca2*r, y+h-r+sa2*r);
	}
	
	gfx_quads_drawTL(x+r, y+r, w-r*2, h-r*2); // center
	gfx_quads_drawTL(x+r, y, w-r*2, r); // top
	gfx_quads_drawTL(x+r, y+h-r, w-r*2, r); // bottom
	gfx_quads_drawTL(x, y+r, r, h-r*2); // left
	gfx_quads_drawTL(x+w-r, y+r, r, h-r*2); // right
}

static void render_player(const obj_player *prev_obj, const obj_player *player_obj)
{
	obj_player prev;
	obj_player player;
	prev = *prev_obj;
	player = *player_obj;
	
	if(player.health < 0) // dont render dead players
		return;
	
	if(player.local)
	{
		// apply predicted results
		predicted_player.write(&player);
		predicted_prev_player.write(&prev);
	}
		
	int skin = charids[player.clientid];
	
	if(gametype != GAMETYPE_DM)
		skin = player.team*9; // 0 or 9

	vec2 direction = get_direction(player.angle);
	float angle = player.angle/256.0f;
	vec2 position = mix(vec2(prev.x, prev.y), vec2(player.x, player.y), client_intratick());
	
	if(prev.health < 0) // Don't flicker from previous position
		position = vec2(player.x, player.y);
	
	bool stationary = player.vx < 1 && player.vx > -1;
	bool inair = col_check_point(player.x, player.y+16) == 0;
	
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

	if (player.weapon == WEAPON_HAMMER)
	{
		float a = clamp((client_tick()-player.attacktick+client_intratick())/10.0f, 0.0f, 1.0f);
		anim_eval_add(&state, &data->animations[ANIM_HAMMER_SWING], a, 1.0f);
	}
	if (player.weapon == WEAPON_NINJA)
	{
		float a = clamp((client_tick()-player.attacktick+client_intratick())/40.0f, 0.0f, 1.0f);
		anim_eval_add(&state, &data->animations[ANIM_NINJA_SWING], a, 1.0f);
	}
		
	// draw hook
	if (prev.hook_state>0 && player.hook_state>0)
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		//gfx_quads_begin();

		vec2 pos = position;
		vec2 hook_pos = mix(vec2(prev.hook_x, prev.hook_y), vec2(player.hook_x, player.hook_y), client_intratick());
		
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

		render_hand(skin, position, normalize(hook_pos-pos), -pi/2, vec2(20, 0));
	}

	// draw gun
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		gfx_quads_setrotation(state.attach.angle*pi*2+angle);

		// normal weapons
		int iw = clamp(player.weapon, 0, NUM_WEAPONS-1);
		select_sprite(data->weapons[iw].sprite_body, direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

		vec2 dir = direction;
		float recoil = 0.0f;
		vec2 p;
		if (player.weapon == WEAPON_HAMMER)
		{
			// Static position for hammer
			p = position;
			p.y += data->weapons[iw].offsety;
			// if attack is under way, bash stuffs
			if(direction.x < 0)
			{
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
				p.x -= data->weapons[iw].offsetx;
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
			}
			draw_sprite(p.x, p.y, data->weapons[iw].visual_size);
		}
		else if (player.weapon == WEAPON_NINJA)
		{
			p = position;
			p.y += data->weapons[iw].offsety;
			
			if(direction.x < 0)
			{
				gfx_quads_setrotation(-pi/2-state.attach.angle*pi*2);
				p.x -= data->weapons[iw].offsetx;
			}
			else
			{
				gfx_quads_setrotation(-pi/2+state.attach.angle*pi*2);
			}
			draw_sprite(p.x, p.y, data->weapons[iw].visual_size);

			// HADOKEN
			if ((client_tick()-player.attacktick) <= (SERVER_TICK_SPEED / 6) && data->weapons[iw].nummuzzlesprites)
			{
				int itex = rand() % data->weapons[iw].nummuzzlesprites;
				float alpha = 1.0f;
				if (alpha > 0.0f && data->weapons[iw].sprite_muzzle[itex].psprite)
				{
					vec2 dir = vec2(player.x,player.y) - vec2(prev.x, prev.y);
					dir = normalize(dir);
					float hadokenangle = atan(dir.y/dir.x);
					if (dir.x < 0.0f)
						hadokenangle += pi;
					gfx_quads_setrotation(hadokenangle);
					//float offsety = -data->weapons[iw].muzzleoffsety;
					select_sprite(data->weapons[iw].sprite_muzzle[itex].psprite, 0);
					vec2 diry(-dir.y,dir.x);
					p = position;
					float offsetx = data->weapons[iw].muzzleoffsetx;
					p -= dir * offsetx;
					draw_sprite(p.x, p.y, 160.0f);
				}
			}
		}
		else
		{
			// TODO: should be an animation
			recoil = 0;
			float a = (client_tick()-player.attacktick+client_intratick())/5.0f;
			if(a < 1)
				recoil = sinf(a*pi);
			p = position + dir * data->weapons[iw].offsetx - dir*recoil*10.0f;
			p.y += data->weapons[iw].offsety;
			draw_sprite(p.x, p.y, data->weapons[iw].visual_size);
		}
		
		if (player.weapon == WEAPON_GUN || player.weapon == WEAPON_SHOTGUN)
		{
			// check if we're firing stuff
			if (true)///prev.attackticks)
			{
				float alpha = 0.0f;
				int phase1tick = (client_tick() - player.attacktick);
				if (phase1tick < (data->weapons[iw].muzzleduration + 3))
				{
					float intratick = client_intratick();
					float t = ((((float)phase1tick) + intratick)/(float)data->weapons[iw].muzzleduration);
					alpha = LERP(2.0, 0.0f, min(1.0f,max(0.0f,t)));
				}

				int itex = rand() % data->weapons[iw].nummuzzlesprites;
				if (alpha > 0.0f && data->weapons[iw].sprite_muzzle[itex].psprite)
				{
					float offsety = -data->weapons[iw].muzzleoffsety;
					select_sprite(data->weapons[iw].sprite_muzzle[itex].psprite, direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
					if(direction.x < 0)
						offsety = -offsety;

					vec2 diry(-dir.y,dir.x);
					p += dir * data->weapons[iw].muzzleoffsetx + diry * offsety;

					draw_sprite(p.x, p.y, data->weapons[iw].visual_size);
					/*gfx_quads_setcolor(1.0f,1.0f,1.0f,alpha);
					vec2 diry(-dir.y,dir.x);
					p += dir * muzzleparams[player.weapon].offsetx + diry * offsety;
					gfx_quads_draw(p.x,p.y,muzzleparams[player.weapon].sizex, muzzleparams[player.weapon].sizey);*/
				}
			}
		}
		gfx_quads_end();

		switch (player.weapon)
		{
			case WEAPON_GUN: render_hand(skin, p, direction, -3*pi/4, vec2(-15, 4)); break;
			case WEAPON_SHOTGUN: render_hand(skin, p, direction, -pi/2, vec2(-5, 4)); break;
			case WEAPON_ROCKET: render_hand(skin, p, direction, -pi/2, vec2(-4, 7)); break;
		}
		
	}

	// render the tee
	if(player.local && config.debug)
	{
		vec2 ghost_position = mix(vec2(prev_obj->x, prev_obj->y), vec2(player_obj->x, player_obj->y), client_intratick());
		render_tee(&state, 15, player.emote, direction, ghost_position); // render ghost
	}
	
	render_tee(&state, skin, player.emote, direction, position);

	if(player.state == STATE_CHATTING)
	{
		gfx_texture_set(data->images[IMAGE_CHAT_BUBBLES].id);
		gfx_quads_begin();
		select_sprite(SPRITE_CHAT_DOTDOT);
		gfx_quads_draw(position.x + 24, position.y - 40, 64,64);
		gfx_quads_end();
	}

	if (client_datas[player.clientid].emoticon_start != -1 && client_datas[player.clientid].emoticon_start + 2 * client_tickspeed() > client_tick())
	{
		gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
		gfx_quads_begin();

		int since_start = client_tick() - client_datas[player.clientid].emoticon_start;
		int from_end = client_datas[player.clientid].emoticon_start + 2 * client_tickspeed() - client_tick();

		float a = 1;

		if (from_end < client_tickspeed() / 5)
			a = from_end / (client_tickspeed() / 5.0);

		float h = 1;
		if (since_start < client_tickspeed() / 10)
			h = since_start / (client_tickspeed() / 10.0);

		float wiggle = 0;
		if (since_start < client_tickspeed() / 5)
			wiggle = since_start / (client_tickspeed() / 5.0);

		float wiggle_angle = sin(5*wiggle);

		gfx_quads_setrotation(pi/6*wiggle_angle);

		gfx_quads_setcolor(1.0f,1.0f,1.0f,a);
		// client_datas::emoticon is an offset from the first emoticon
		select_sprite(SPRITE_OOP + client_datas[player.clientid].emoticon);
		gfx_quads_draw(position.x, position.y - 23 - 32*h, 64, 64*h);
		gfx_quads_end();
	}
}

void render_sun(float x, float y)
{
	vec2 pos(x, y);

	gfx_texture_set(-1);
	gfx_blend_additive();
	gfx_quads_begin();
	const int rays = 10;
	gfx_quads_setcolor(1.0f,1.0f,1.0f,0.025f);
	for(int r = 0; r < rays; r++)
	{
		float a = r/(float)rays + client_localtime()*0.025f;
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

static bool emoticon_selector_inactive_override = false;
static vec2 emoticon_selector_mouse;

void emoticon_selector_reset()
{
	emoticon_selector_mouse = vec2(0, 0);
}

int emoticon_selector_render()
{
	int x, y;
	inp_mouse_relative(&x, &y);

	emoticon_selector_mouse.x += x;
	emoticon_selector_mouse.y += y;

	if (length(emoticon_selector_mouse) > 70)
		emoticon_selector_mouse = normalize(emoticon_selector_mouse) * 70;

	float selected_angle = get_angle(emoticon_selector_mouse) + 2*pi/24;
	if (selected_angle < 0)
		selected_angle += 2*pi;

	static bool mouse_down = false;
	bool return_now = false;
	int selected_emoticon; 

	if (length(emoticon_selector_mouse) < 50)
		selected_emoticon = -1;
	else
		selected_emoticon = (int)(selected_angle / (2*pi) * 12.0f);

	if (inp_key_pressed(KEY_MOUSE_1))
	{
		mouse_down = true;
	}
	else if (mouse_down)
	{
		mouse_down = false;

		if (selected_emoticon != -1)
		{
			return_now = true;
			emoticon_selector_active = false;
			emoticon_selector_inactive_override = true;
		}
	}

	gfx_mapscreen(0,0,400,300);

	gfx_blend_normal();
	
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_quads_setcolor(0,0,0,0.3f);
	draw_circle(200, 150, 80, 64);
	gfx_quads_end();

	gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
	gfx_quads_begin();

	for (int i = 0; i < 12; i++)
	{
		float angle = 2*pi*i/12.0;
		if (angle > pi)
			angle -= 2*pi;

		bool selected = selected_emoticon == i;

		float size = selected ? 48 : 32;

		float nudge_x = 60 * cos(angle);
		float nudge_y = 60 * sin(angle);
		select_sprite(SPRITE_OOP + i);
		gfx_quads_draw(200 + nudge_x, 150 + nudge_y, size, size);
	}

	gfx_quads_end();

    gfx_texture_set(data->images[IMAGE_CURSOR].id);
    gfx_quads_begin();
    gfx_quads_setcolor(1,1,1,1);
    gfx_quads_drawTL(emoticon_selector_mouse.x+200,emoticon_selector_mouse.y+150,12,12);
    gfx_quads_end();

	return return_now ? selected_emoticon : -1;
}

void render_scoreboard(obj_game *gameobj, float x, float y, float w, int team, const char *title)
{
	//float w = 550.0f;
	//float x = width/2-w/2;
	//;
	//float y = ystart;
	//float w = 550.0f;

	animstate idlestate;
	anim_eval(&data->animations[ANIM_BASE], 0, &idlestate);
	anim_eval_add(&idlestate, &data->animations[ANIM_IDLE], 0, 1.0f);	
	
	float ystart = y;
	float h = 600.0f;

	gfx_blend_normal();
	
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_quads_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 40.0f);
	gfx_quads_end();
	
	// render title
	if(!title)
	{
		if(gameobj->game_over)
			title = "Game Over";
		else
			title = "Score Board";
	}

	float tw = gfx_pretty_text_width( 64, "Game Over", -1);
	gfx_pretty_text(x+w/2-tw/2, y, 64, "Game Over", -1);
	

	y += 64.0f;

	// find players
	const obj_player *players[MAX_CLIENTS] = {0};
	int num_players = 0;
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);
		
		if(item.type == OBJTYPE_PLAYER)
		{
			players[num_players] = (const obj_player *)data;
			num_players++;
		}
	}
	
	// sort players
	for(int k = 0; k < num_players; k++) // ffs, bubblesort
	{
		for(int i = 0; i < num_players-k-1; i++)
		{
			if(players[i]->score < players[i+1]->score)
			{
				const obj_player *tmp = players[i];
				players[i] = players[i+1];
				players[i+1] = tmp;
			}
		}
	}

	// render headlines
	gfx_pretty_text(x+10, y, 32, "Score", -1);
	gfx_pretty_text(x+125, y, 32, "Name", -1);
	gfx_pretty_text(x+w-70, y, 32, "Ping", -1);
	y += 38.0f;

	// render player scores
	for(int i = 0; i < num_players; i++)
	{
		const obj_player *player = players[i];
		
		// make sure that we render the correct team
		if(team != -1 && player->team != team)
			continue;
		
		char buf[128];
		float font_size = 46.0f;
		if(player->local)
		{
			// background so it's easy to find the local player
			gfx_texture_set(-1);
			gfx_quads_begin();
			gfx_quads_setcolor(1,1,1,0.25f);
			draw_round_rect(x, y, w-20, 48, 20.0f);
			gfx_quads_end();
		}
		
		sprintf(buf, "%4d", player->score);
		gfx_pretty_text(x+60-gfx_pretty_text_width(font_size,buf,-1), y, font_size, buf, -1);
		gfx_pretty_text(x+128, y, font_size, client_datas[player->clientid].name, -1);
		
		sprintf(buf, "%4d", player->latency);
		float tw = gfx_pretty_text_width(font_size, buf, -1);
		gfx_pretty_text(x+w-tw-35, y, font_size, buf, -1);

		// render avatar
		render_tee(&idlestate, player->clientid, EMOTE_NORMAL, vec2(1,0), vec2(x+90, y+28));
		y += 50.0f;
	}
	
	// render goals
	y = ystart+h-54;
	if(gameobj && gameobj->time_limit)
	{
		char buf[64];
		sprintf(buf, "Time Limit: %d min", gameobj->time_limit);
		gfx_pretty_text(x+w/2, y, 32, buf, -1);
	}
	if(gameobj && gameobj->score_limit)
	{
		char buf[64];
		sprintf(buf, "Score Limit: %d", gameobj->score_limit);
		gfx_pretty_text(x+40, y, 32, buf, -1);
	}				
}

void render_game()
{	
	animstate idlestate;
	anim_eval(&data->animations[ANIM_BASE], 0, &idlestate);
	anim_eval_add(&idlestate, &data->animations[ANIM_IDLE], 0, 1.0f);	

	if (inp_key_down(KEY_ESC))
	{
		if (chat_active)
			chat_active = false;
		else
			menu_active = !menu_active;
	}
	
	if (!menu_active)
	{
		if(inp_key_down(KEY_ENTER))
		{
			if(chat_active)
			{
				// send message
				if(chat_input_len)
				{
					if(chat_input[0] == '/')
						config_set(&chat_input[1]);
					else
					{
						msg_pack_start(MSG_SAY, MSGFLAG_VITAL);
						msg_pack_string(chat_input, 512);
						msg_pack_end();
						client_send_msg();
					}
				}
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
			int c = inp_last_char();
			int k = inp_last_key();
		
			if (!(c >= 0 && c < 32))
			{
				if (chat_input_len < sizeof(chat_input) - 1)
				{
					chat_input[chat_input_len] = c;
					chat_input[chat_input_len+1] = 0;
					chat_input_len++;
				}
			}

			if(k == KEY_BACKSPACE)
			{
				if(chat_input_len > 0)
				{
					chat_input[chat_input_len-1] = 0;
					chat_input_len--;
				}
			}
			
		}
	}
	
	if (!menu_active)
	{
		inp_clear();
	}
	
	// fetch new input
	if(!menu_active && (!emoticon_selector_active || emoticon_selector_inactive_override))
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
			
		input.target_x = (int)mouse_pos.x;
		input.target_y = (int)mouse_pos.y;
		input.activeweapon = -1;
	
		if(chat_active)
			input.state = STATE_CHATTING;
		else if(menu_active)
			input.state = STATE_IN_MENU;
		else
		{
			input.state = STATE_PLAYING;
			input.left = inp_key_pressed(config.key_move_left);
			input.right = inp_key_pressed(config.key_move_right);
			input.jump = inp_key_pressed(config.key_jump);
			// TODO: this is not very well done. it should check this some other way
			input.fire = emoticon_selector_active ? 0 : inp_key_pressed(config.key_fire);
			input.hook = inp_key_pressed(config.key_hook);

			//input.blink = inp_key_pressed('S');
			// Weapon switching
			if(inp_key_pressed(config.key_weapon1)) input.activeweapon = 0;
			if(inp_key_pressed(config.key_weapon2)) input.activeweapon = 1;
			if(inp_key_pressed(config.key_weapon3)) input.activeweapon = 2;
			if(inp_key_pressed(config.key_weapon4)) input.activeweapon = 3;
		}
		
		// stress testing
		if(config.stress)
		{
			float t = client_localtime();
			mem_zero(&input, sizeof(input));
			input.left = 1;
			input.jump = ((int)t)&1;
			input.fire = ((int)(t*10))&1;
			input.hook = ((int)t)&1;
			input.activeweapon = ((int)t)%NUM_WEAPONS;
			input.target_x = (int)(sinf(t*3)*100.0f);
			input.target_y = (int)(cosf(t*3)*100.0f);
			
			//input.target_x = (int)((rand()/(float)RAND_MAX)*64-32);
			//input.target_y = (int)((rand()/(float)RAND_MAX)*64-32);
			
		}

		snap_input(&input, sizeof(input));
	}

	// setup world view
	obj_game *gameobj = 0;
	{
		// 1. fetch local player
		// 2. set him to the center
		int num = snap_num_items(SNAP_CURRENT);
		for(int i = 0; i < num; i++)
		{
			SNAP_ITEM item;
			const void *data = snap_get_item(SNAP_CURRENT, i, &item);
			
			if(item.type == OBJTYPE_PLAYER)
			{
				obj_player *player = (obj_player *)data;
				if(player->local)
				{
					local_player = player;
					local_player_pos = vec2(player->x, player->y);

					const void *p = snap_find_item(SNAP_PREV, item.type, item.id);
					if(p)
						local_player_pos = mix(vec2(((obj_player *)p)->x, ((obj_player *)p)->y), local_player_pos, client_intratick());
				}
			}
			else if(item.type == OBJTYPE_GAME)
				gameobj = (obj_game *)data;
		}
	}
	
	local_player_pos = mix(predicted_prev_player.pos, predicted_player.pos, client_intratick());
	//local_player_pos = predicted_player.pos;
	
	// everything updated, do events
	if(must_process_events)
		process_events(SNAP_PREV);

	// pseudo format
	float zoom = 3.0f;
	
	if(config.debug)
	{
		if(!chat_active && inp_key_pressed(KEY_F8))
			zoom = 1.0f;
	}
	
	float width = 400*zoom;
	float height = 300*zoom;
	float screen_x = 0;
	float screen_y = 0;
	
	// center at char but can be moved when mouse is far away
	float offx = 0, offy = 0;
	if (config.dynamic_camera)
	{
		int deadzone = 300;
		if(mouse_pos.x > deadzone) offx = mouse_pos.x-deadzone;
		if(mouse_pos.x <-deadzone) offx = mouse_pos.x+deadzone;
		if(mouse_pos.y > deadzone) offy = mouse_pos.y-deadzone;
		if(mouse_pos.y <-deadzone) offy = mouse_pos.y+deadzone;
		offx = offx*2/3;
		offy = offy*2/3;
	}
		
	screen_x = local_player_pos.x+offx;
	screen_y = local_player_pos.y+offy;
	
	gfx_mapscreen(screen_x-width/2, screen_y-height/2, screen_x+width/2, screen_y+height/2);
	
	// draw background
	gfx_clear(0.65f,0.78f,0.9f);

	// draw the sun
	if(config.gfx_high_detail)
	{
		render_sun(20+screen_x*0.6f, 20+screen_y*0.6f);
		
		static vec2 cloud_pos[6] = {vec2(-500,0),vec2(-500,200),vec2(-500,400)};
		static float cloud_speed[6] = {30, 20, 10};
		static int cloud_sprites[6] = {SPRITE_CLOUD1, SPRITE_CLOUD2, SPRITE_CLOUD3};
		
		gfx_texture_set(data->images[IMAGE_CLOUDS].id);
		gfx_quads_begin();
		for(int i = 0; i < 3; i++)
		{
			float parallax_amount = 0.55f;
			select_sprite(cloud_sprites[i]);
			draw_sprite((cloud_pos[i].x+fmod(client_localtime()*cloud_speed[i]+i*100.0f, 3000.0f))+screen_x*parallax_amount,
				cloud_pos[i].y+screen_y*parallax_amount, 300);
		}
		gfx_quads_end();

		
		// draw backdrop
		gfx_texture_set(data->images[IMAGE_BACKDROP].id);
		gfx_quads_begin();
		float parallax_amount = 0.25f;
		for(int x = -1; x < 3; x++)
			gfx_quads_drawTL(1024*x+screen_x*parallax_amount, (screen_y)*parallax_amount+150+512, 1024, 512);
		gfx_quads_end();
	}
	
	// render map
	tilemap_render(32.0f, 0);
	
	// render items
	int num = snap_num_items(SNAP_CURRENT);
	for(int i = 0; i < num; i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);
		
		if(item.type == OBJTYPE_PLAYER)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
			{
				client_datas[((const obj_player *)data)->clientid].team = ((const obj_player *)data)->team;
				render_player((const obj_player *)prev, (const obj_player *)data);
			}
		}
		else if(item.type == OBJTYPE_PROJECTILE)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_projectile((const obj_projectile *)prev, (const obj_projectile *)data, item.id);
		}
		else if(item.type == OBJTYPE_POWERUP)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_powerup((const obj_powerup *)prev, (const obj_powerup *)data);
		}
		else if(item.type == OBJTYPE_FLAG)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if (prev)
				render_flag((const obj_flag *)prev, (const obj_flag *)data);
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
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		
		// render cursor
		if (!menu_active && (!emoticon_selector_active || emoticon_selector_inactive_override))
		{
			select_sprite(data->weapons[local_player->weapon%data->num_weapons].sprite_cursor);
			float cursorsize = 64;
			draw_sprite(local_player_pos.x+mouse_pos.x, local_player_pos.y+mouse_pos.y, cursorsize);
		}

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
	
	// render kill messages
	{
		gfx_mapscreen(0, 0, width*1.5f, height*1.5f);
		float startx = width*1.5f-10.0f;
		float y = 10.0f;
		
		for(int i = 0; i < killmsg_max; i++)
		{
			
			int r = (killmsg_current+i+1)%killmsg_max;
			if(client_tick() > killmsgs[r].tick+50*10)
				continue;
			
			float font_size = 48.0f;
			float killername_w = gfx_pretty_text_width(font_size, client_datas[killmsgs[r].killer].name, -1);
			float victimname_w = gfx_pretty_text_width(font_size, client_datas[killmsgs[r].victim].name, -1);
			
			float x = startx;
			
			// render victim name
			x -= victimname_w;
			gfx_pretty_text(x, y, font_size, client_datas[killmsgs[r].victim].name, -1);
			
			// render victim tee
			x -= 24.0f;
			int skin = gametype == GAMETYPE_TDM ? skinseed + client_datas[killmsgs[r].victim].team : killmsgs[r].victim;
			render_tee(&idlestate, skin, EMOTE_PAIN, vec2(-1,0), vec2(x, y+28));
			x -= 32.0f;

			// render weapon
			x -= 44.0f;
			if (killmsgs[r].weapon >= 0)
			{
				gfx_texture_set(data->images[IMAGE_GAME].id);
				gfx_quads_begin();
				select_sprite(data->weapons[killmsgs[r].weapon].sprite_body);
				draw_sprite(x, y+28, 96);
				gfx_quads_end();
			}
			x -= 52.0f;

			// render killer tee
			x -= 24.0f;
			skin = gametype == GAMETYPE_TDM ? skinseed + client_datas[killmsgs[r].killer].team : killmsgs[r].killer;
			render_tee(&idlestate, skin, EMOTE_ANGRY, vec2(1,0), vec2(x, y+28));
			x -= 32.0f;
			
			// render killer name
			x -= killername_w;
			gfx_pretty_text(x, y, font_size, client_datas[killmsgs[r].killer].name, -1);
			
			y += 44;
		}
	}
	
	// render chat
	{
		gfx_mapscreen(0,0,400,300);
		float x = 10.0f;
		float y = 300.0f-50.0f;
		float starty = -1;
		if(chat_active)
		{
			// render chat input
			char buf[sizeof(chat_input)+16];
			sprintf(buf, "Chat: %s_", chat_input);
			gfx_pretty_text(x, y, 10.0f, buf, 380);
			starty = y;
		}
		
		y -= 10;
		
		int i;
		for(i = 0; i < chat_max_lines; i++)
		{
			int r = ((chat_current_line-i)+chat_max_lines)%chat_max_lines;
			if(client_tick() > chat_lines[r].tick+50*15)
				break;

			int lines = int(gfx_pretty_text_width(10, chat_lines[r].text, -1)) / 380 + 1;
			
			gfx_pretty_text(x, y - 8 * (lines - 1), 10, chat_lines[r].text, 380);
			y -= 8 * lines;
		}
	}
	
	// render goals
	if(gameobj)
	{
		gametype = gameobj->gametype;
		gfx_mapscreen(0,0,400,300);
		if(!gameobj->sudden_death)
		{
			char buf[32];
			int time = 0;
			if(gameobj->time_limit)
			{
				time = gameobj->time_limit*60 - ((client_tick()-gameobj->round_start_tick)/client_tickspeed());
				
				if(gameobj->game_over)
					time  = 0;
			}	
			else
				time = (client_tick()-gameobj->round_start_tick)/client_tickspeed();
			
			sprintf(buf, "%d:%02d", time /60, time %60);
			float w = gfx_pretty_text_width(16, buf, -1);
			gfx_pretty_text(200-w/2, 2, 16, buf, -1);
		}

		if(gameobj->sudden_death)
		{
			const char *text = "Sudden Death";
			float w = gfx_pretty_text_width(16, text, -1);
			gfx_pretty_text(200-w/2, 2, 16, text, -1);
		}
	}

	if (menu_active)
	{
		if (modmenu_render(true))
			menu_active = false;
			
		//ingamemenu_render();
		return;
	}

	if (inp_key_pressed(config.key_emoticon))
	{
		if (!emoticon_selector_active)
		{
			emoticon_selector_active = true;
			emoticon_selector_reset();
		}
	}
	else
	{
		emoticon_selector_active = false;
		emoticon_selector_inactive_override = false;
	}

	if (emoticon_selector_active && !emoticon_selector_inactive_override)
	{
		int emoticon = emoticon_selector_render();
		if (emoticon != -1)
			send_emoticon(emoticon);
	}
	
	// render score board
	if(inp_key_pressed(KEY_TAB) || // user requested
		(local_player && local_player->health == -1) || // player dead
		(gameobj && gameobj->game_over) // game over
		)
	{
		gfx_mapscreen(0, 0, width, height);

		float w = 550.0f;
		
		if (gameobj && gameobj->gametype == GAMETYPE_DM)
		{
			render_scoreboard(gameobj, width/2-w/2, 150.0f, w, -1, 0);
			//render_scoreboard(gameobj, 0, 0, -1, 0);
		}
		else
		{
			render_scoreboard(gameobj, width/2-w-20, 150.0f, w, 0, "Team A");
			render_scoreboard(gameobj, width/2 + 20, 150.0f, w, 1, "Team B");
		}

	}
}

extern "C" void modc_render()
{
	// this should be moved around abit
	if(client_state() == CLIENTSTATE_ONLINE)
	{
		if (music_menu_id != -1)
		{
			snd_stop(music_menu_id);
			music_menu_id = -1;
		}
		
		render_game();
	}
	else // if (client_state() != CLIENTSTATE_CONNECTING && client_state() != CLIENTSTATE_LOADING)
	{
		if (music_menu_id == -1)
			music_menu_id = snd_play(music_menu, SND_LOOP, 1.0f, 0.0f);
		
		//netaddr4 server_address;
		if(modmenu_render(false) == -1)
			client_quit();
	}
}


void menu_do_disconnected();
void menu_do_connecting();
void menu_do_connected();

extern "C" void modc_statechange(int state, int old)
{
	if(state == CLIENTSTATE_OFFLINE)
	 	menu_do_disconnected();
	if(state == CLIENTSTATE_CONNECTING)
		menu_do_connecting();
	if (state == CLIENTSTATE_ONLINE)
		menu_do_connected();
}

extern "C" void modc_message(int msg)
{
	if(msg == MSG_CHAT)
	{
		int cid = msg_unpack_int();
		const char *message = msg_unpack_string();
		dbg_msg("message", "chat cid=%d msg='%s'", cid, message);
		chat_add_line(cid, message);

		if(cid >= 0)
			snd_play(data->sounds[SOUND_CHAT_CLIENT].sounds[0].id, SND_PLAY_ONCE, 1.0f, 0.0f);
		else
			snd_play(data->sounds[SOUND_CHAT_SERVER].sounds[0].id, SND_PLAY_ONCE, 1.0f, 0.0f);
	}
	else if(msg == MSG_SETNAME)
	{
		int cid = msg_unpack_int();
		const char *name = msg_unpack_string();
		strncpy(client_datas[cid].name, name, 64);
	}
	else if(msg == MSG_KILLMSG)
	{
		killmsg_current = (killmsg_current+1)%killmsg_max;
		killmsgs[killmsg_current].killer = msg_unpack_int();
		killmsgs[killmsg_current].victim = msg_unpack_int();
		killmsgs[killmsg_current].weapon = msg_unpack_int();
		killmsgs[killmsg_current].tick = client_tick();
	}
	else if (msg == MSG_EMOTICON)
	{
		int cid = msg_unpack_int();
		int emoticon = msg_unpack_int();
		client_datas[cid].emoticon = emoticon;
		client_datas[cid].emoticon_start = client_tick();
	}
}


extern "C" const char *modc_net_version() { return TEEWARS_NETVERSION; }
