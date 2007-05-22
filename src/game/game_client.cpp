#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "mapres_image.h"
#include "mapres_tilemap.h"

using namespace baselib;

static int texture_char_default = 0;
static int texture_game = 0;
static int texture_weapon = 0;
static int texture_sun = 0;
static int texture_particles = 0;

struct weapontexcell
{
	float x;
	float y;
	float w;
	float h;
};
struct renderparams
{
	float sizex;
	float sizey;
	float offsetx;
	float offsety;
};
int numcellsx = 32;
int numcellsy = 32;
renderparams weaponrenderparams[WEAPON_NUMWEAPONS];
renderparams modifierrenderparams[WEAPON_NUMWEAPONS];

weapontexcell weaponprojtexcoord[WEAPON_NUMWEAPONS];
weapontexcell weapontexcoord[WEAPON_NUMWEAPONS];
weapontexcell weapontexcoordcursor[WEAPON_NUMWEAPONS];

weapontexcell poweruptexcoord[POWERUP_TYPE_NUMPOWERUPS];

weapontexcell modifiertexcoord[MODIFIER_NUMMODIFIERS];
weapontexcell modifiertexcoordcursor[MODIFIER_NUMMODIFIERS];

int nummuzzletex[WEAPON_NUMWEAPONS];
weapontexcell muzzletexcoord[WEAPON_NUMWEAPONS][3];
renderparams muzzleparams[WEAPON_NUMWEAPONS];

#define NUMHADOKENS 6
#define NUMSTARS 2
#define NUMPARTICLES 9
int particlesnumcellsx = 16;
int particlesnumcellsy = 16;
weapontexcell chaintexcoord;
weapontexcell chainheadtexcoord;
weapontexcell stars[NUMSTARS];

float lifemodifier[NUMPARTICLES];
vec4 particlecolors[NUMPARTICLES];
weapontexcell particlestexcoord[NUMPARTICLES];

int charnumcellsx = 8;
int charnumcellsy = 32;
int charoffsety = 2;
weapontexcell body[2];
weapontexcell leye;
weapontexcell reye;
weapontexcell feet[2];

int charids[16] = { 2,10,0,4,12,6,14,1,9,15,13,11,7,5,8,3 };

renderparams hadokenparams[6];
weapontexcell hadoken[6];

float recoils[WEAPON_NUMWEAPONS] = { 10.0f, 10.0f, 10.0f, 10.0f };

static int font_texture = 0;
static vec2 mouse_pos;


static vec2 local_player_pos;
static obj_player *local_player;

float frandom()
{
	return rand()/(float)(RAND_MAX);
}

float sign(float f)
{
	return f<0.0f?-1.0f:1.0f;
}

// sound helpers
template<int N>
class sound_kit
{
private:
	int sounds[N];
	int last_id;
public:
	sound_kit() : last_id(-1) { }
	
	int& operator[](int id) { return sounds[id]; }

	inline void play_random(float vol = 1.0f, float pan = 0.0f);
};

template<>
inline void sound_kit<1>::play_random(float vol, float pan)
{
	snd_play(sounds[0], SND_PLAY_ONCE, vol, pan);
}

template<int N>
inline void sound_kit<N>::play_random(float vol, float pan)
{
	int id;
	do {
		id = rand() % N;
	} while(id == last_id);
	snd_play(sounds[id], SND_PLAY_ONCE, vol, pan);
	last_id = id;
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

// sounds
sound_kit<3> sound_gun_fire;
sound_kit<3> sound_shotty_fire;
sound_kit<3> sound_flump_launch;
sound_kit<3> sound_hammer_swing;
sound_kit<3> sound_ninja_attack;

sound_kit<3> sound_flump_explode;
sound_kit<4> sound_ninja_hit;

sound_kit<3> sound_weapon_switch;

sound_kit<12> sound_pain_short;
sound_kit<2> sound_pain_long;

sound_kit<4> sound_body_jump;
sound_kit<4> sound_body_land;
sound_kit<2> sound_body_splat;

sound_kit<7> sound_spawn;
sound_kit<2> sound_tee_cry;

sound_kit<1> sound_hook_loop;
sound_kit<3> sound_hook_attach;

void sound_vol_pan(const vec2& p, float *vol, float *pan)
{
	vec2 player_to_ev = p - local_player_pos;
	*pan = 0.0f;
	*vol = 1.0f;

	if(abs(player_to_ev.x) > stereo_separation_deadzone)
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

// TODO: we should do something nicer then this
static void cell_select_ex(int cx, int cy, float x, float y, float w, float h)
{
	gfx_quads_setsubset(x/(float)cx,y/(float)cy,(x+w)/(float)cx,(y+h)/(float)cy);
}

static void cell_select_ex_flip_x(int cx, int cy, float x, float y, float w, float h)
{
	gfx_quads_setsubset((x+w)/(float)cx,y/(float)cy,x /(float)cx,(y+h)/(float)cy);
}

static void cell_select_ex_flip_y(int cx, int cy, float x, float y, float w, float h)
{
	gfx_quads_setsubset(x/(float)cx,(y+h)/(float)cy,(x+w)/(float)cx,y/(float)cy);
}

static void cell_select(int x, int y, int w, int h)
{
	gfx_quads_setsubset(x/16.0f,y/16.0f,(x+w)/16.0f,(y+h)/16.0f);
}

inline void cell_select_flip_x(int x, int y, int w, int h)
{
	gfx_quads_setsubset((x+w)/16.0f,y/16.0f,(x)/16.0f,(y+h)/16.0f);
}

inline void cell_select_flip_y(int x, int y, int w, int h)
{
	gfx_quads_setsubset(x/16.0f,(y+h)/16.0f,(x+w)/16.0f,(y)/16.0f);
}

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

class health_texts
{
public:
	int64 lastupdate;
	struct item
	{
		vec2 pos;
		vec2 vel;
		int amount;
		int istar;
		float life;
		float startangle;
	};
	
	enum
	{
		MAX_ITEMS=16,
	};

	health_texts()
	{
		lastupdate = 0;
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
	
	void create(vec2 pos, int amount)
	{
		amount = max(1,amount);
		for (int j = 0; j < amount; j++)
		{
			float a = j/(float)amount-0.5f;
			item *i = create_i();
			if (i)
			{
				i->pos = pos;
				i->pos.y -= 20.0f;
				i->pos.x += ((float)rand()/(float)RAND_MAX) * 5.0f;
				i->amount = amount;
				i->life = 1.5f;
				i->istar = rand() % NUMSTARS;
				i->vel = vec2(((float)rand()/(float)RAND_MAX) * 50.0f,-150.0f);
				i->startangle = (( (float)rand()/(float)RAND_MAX) - 1.0f) * 2.0f * pi;
			}
		}
	}
	
	void render()
	{
		if (!lastupdate)
			lastupdate = time_get();

		int64 lasttime = lastupdate;
		lastupdate = time_get();

		float delta = (float) (lastupdate - lasttime) / (float)time_freq();
		gfx_texture_set(texture_particles);
		gfx_quads_begin();
		for(int i = 0; i < num_items;)
		{
			items[i].vel += vec2(0,500.0f) * delta;
			items[i].pos += items[i].vel * delta;
			items[i].life -= delta;
			//items[i].pos.y -= frametime*15.0f;
			if(items[i].life < 0.0f)
				destroy_i(&items[i]);
			else
			{
				gfx_quads_setcolor(1.0f,1.0f,1.0f, items[i].life / 1.5f);
				gfx_quads_setrotation(items[i].startangle + items[i].life * 2.0f);
				float size = 64.0f;
				cell_select_ex(particlesnumcellsx,particlesnumcellsy, stars[items[i].istar].x,stars[items[i].istar].y, stars[items[i].istar].w, stars[items[i].istar].h);
				gfx_quads_draw(items[i].pos.x-size/2, items[i].pos.y-size/2, size, size);
				/*char buf[32];
				if(items[i].amount < 0)
				{
					sprintf(buf, "%d", items[i].amount*-1);
				}
				else
				{
					sprintf(buf, "%d", items[i].amount);
				}
				float size = 42.0f;
				if(items[i].life > 1.25f)
					size += 42.0f * ((items[i].life - 1.25f) * 4);
				gfx_quads_text(items[i].pos.x-size/2, items[i].pos.y, size, buf);*/
				i++;
			}
		}
		gfx_quads_end();
	}
	
};

/*class texture_animator
{
public:
	
	int texture;
	int numframes;
	float duration;
	float* framereltime;
	weapontexcell* params;
	texture_animator()
	{
		texture = -1;
		numframes = 0;
		duration = 0;
		framereltime = 0;
		params = 0;
	}

	~texture_animator()
	{
		if (params)
			mem_free(params);
		if (framereltime)
			mem_free(framereltime);
	}

	void create_anim(int texture, int numframes, float duration)
	{
		framereltime = 0;
		params = 0;
		this->texture = texture;
		this->numframes = numframes;
		this->duration = duration;
		if (numframes)
		{
			framereltime = (float*)mem_alloc(sizeof(float) * numframes,1);
			params = (weapontexcell*)mem_alloc(sizeof(renderparams) * numframes,1);
			float delta = 1.0f / (float)(numframes - 1);
			for (int i = 0; i < numframes; i++)
			{
				framereltime[i] = delta * i;
			}
		}	
	}

	static void create_gunmuzzle(texture_animator& anim, int texture, float duration)
	{
		anim.create_anim(texture, 3, duration);
		anim.params[0].x = 8;
		anim.params[0].y = 4;
		anim.params[0].w = 3;
		anim.params[0].h = 2;
		anim.params[1].x = 12;
		anim.params[1].y = 4;
		anim.params[1].w = 3;
		anim.params[1].h = 2;
		anim.params[2].x = 16;
		anim.params[2].y = 4;
		anim.params[2].w = 3;
		anim.params[2].h = 2;
	}

	static void create_shotgunmuzzle()
	{

	}
};*/

class keyframe
{
public:
	vec2 pos;
	float angle;
	float relativetime;
};

class anim
{
public:
	keyframe* keyframes;
	int numframes;
	float duration;
	anim()
	{
		numframes = 0;
		keyframes = 0;
	}
	~anim()
	{
		if (keyframes)
			mem_free(keyframes);
	}

	void create_anim(int numframes, float duration)
	{
		if (keyframes)
			mem_free(keyframes);

		this->numframes = numframes;
		this->duration = duration;
		keyframes = (keyframe*)mem_alloc(sizeof(keyframe) * numframes,1);
		float delta = 1.0f / (float) (numframes - 1);
		for (int i = 0; i < numframes; i++)
		{
			keyframes[i].pos = vec2(0.0f,0.0f);
			keyframes[i].angle = 0;
			keyframes[i].relativetime = delta * (float)i;
		}
	}

	void getframes(float relativetime, keyframe*& frame1, keyframe*& frame2, float& blend)
	{
		for (int i = 1; i < numframes; i++)
		{
			if (keyframes[i-1].relativetime <= relativetime && keyframes[i].relativetime >= relativetime)
			{
				frame1 = &keyframes[i-1];
				frame2 = &keyframes[i];
				blend = (relativetime - frame1->relativetime) / (frame2->relativetime - frame1->relativetime);
			}
		}
	}

	void evalanim(float time, vec2& pos, float& angle)
	{
		float reltime = max(0.0f, min(1.0f, time / duration));
		keyframe* frame1 = 0;
		keyframe* frame2 = 0;
		float blend = 0.0f;
		getframes(reltime, frame1, frame2, blend);

		if (frame1 && frame2)
		{
			pos = mix(frame1->pos, frame2->pos, blend);
			angle = LERP(frame1->angle, frame2->angle, blend);
		}
	}

	static void setup_hammer(anim& hammeranim)
	{
		// straight up = -0.25
		// frame0 = standard pose		time 0
		// frame1 = back a little		time 0.3
		// frame2 = over head			time 0.4
		// frame3 = on ground smashed	time 0.5
		// frame4 = back to standard pose time 1.0
		hammeranim.create_anim(5, 1.0f);
		// only angles... (for now...)
		hammeranim.keyframes[0].angle = -0.35f * pi * 2.0f;
		hammeranim.keyframes[1].angle = -0.4f * pi * 2.0f;
		hammeranim.keyframes[1].relativetime = 0.3f;
		hammeranim.keyframes[2].angle = -0.25f;
		hammeranim.keyframes[2].relativetime = 0.4f;
		hammeranim.keyframes[3].angle = 0.0f * pi * 2.0f;
		hammeranim.keyframes[3].relativetime = 0.5f;
		hammeranim.keyframes[4].angle = -0.35f * pi * 2.0f;
		hammeranim.keyframes[4].relativetime = 1.0f;
	}

	static void setup_ninja(anim& ninjanim)
	{
		// (straight up = -0.25)
		// frame0 = standard pose straight back		time 0.0
		// frame1 = overhead attack frame 1			time 0.1
		// frame2 = attack end frame				time 0.15
		// frame3 = attack hold frame (a bit up)	time 0.4
		// frame4 = attack hold frame end			time 0.7
		// frame5 = endframe						time 1.0
		ninjanim.create_anim(6, 1.0f);
		// only angles... (for now...)
		ninjanim.keyframes[0].angle = -0.5f * pi * 2.0f;

		ninjanim.keyframes[1].angle = -0.3f * pi * 2.0f;
		ninjanim.keyframes[1].relativetime = 0.1f;

		ninjanim.keyframes[2].angle = 0.1f * pi * 2.0f;
		ninjanim.keyframes[2].relativetime = 0.15f;

		ninjanim.keyframes[3].angle = -0.05f * pi * 2.0f;
		ninjanim.keyframes[3].relativetime = 0.42;

		ninjanim.keyframes[4].angle = -0.05f * pi * 2.0f;
		ninjanim.keyframes[4].relativetime = 0.5f;

		ninjanim.keyframes[5].angle = -0.5f * pi * 2.0f;
		ninjanim.keyframes[5].relativetime = 1.0f;
	}
};

static anim hammeranim;
static anim ninjaanim;
static health_texts healthmods;

class particle_system
{
public:
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

		particles[num_particles].iparticle = rand() % NUMPARTICLES;
		particles[num_particles].pos = pos;
		particles[num_particles].vel = vel;
		particles[num_particles].life = life - lifemodifier[particles[num_particles].iparticle] * life;
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
		gfx_texture_set(texture_particles);
		gfx_quads_begin();
		//cell_select(4,1,1,1);
		//cell_select(0,6,2,2);
		//gfx_quads_setrotation(get_angle(vec2(proj->vx, proj->vy)));
		for(int i = 0; i < num_particles; i++)
		{
			int type = particles[i].iparticle;
			cell_select_ex(particlesnumcellsx,particlesnumcellsy,particlestexcoord[type].x, particlestexcoord[type].y, particlestexcoord[type].w, particlestexcoord[type].h);
			float a = 1 - particles[i].life / particles[i].max_life;
			vec2 p = particles[i].pos;
			//a *= length(particles[i].vel) * 0.01f;
			gfx_quads_setrotation(particles[i].rot);
			gfx_quads_setcolor(particlecolors[type].x,particlecolors[type].y,particlecolors[type].z,pow(a,0.75f));
			//gfx_quads_setcolor(particlecolors[type].x * 0.5,particlecolors[type].y * 0.5,particlecolors[type].z* 0.5,pow(a,0.75f));
			//gfx_quads_setcolor(particlecolors[type].x * 0.0,particlecolors[type].y * 0.0,particlecolors[type].z* 0.0,pow(a,0.75f));
			//gfx_quads_setcolor(0.64f*2,0.28f*2,0.16f*2,pow(a,0.75f));
			gfx_quads_draw(p.x, p.y,particles[i].size,particles[i].size);
		}
		gfx_quads_end();		
		gfx_blend_normal();
	}
};

static particle_system temp_system;
 
void modc_init()
{
	// load textures
	texture_weapon = gfx_load_texture_tga("data/tileset_weapons.tga");
	texture_game = gfx_load_texture_tga("data/game_main.tga");
	texture_char_default = gfx_load_texture_tga("data/char_teefault.tga");
	texture_sun = gfx_load_texture_tga("data/sun.tga");
	texture_particles = gfx_load_texture_tga("data/tileset_particles.tga");
	font_texture = gfx_load_texture_tga("data/debug_font.tga");

	
	// load sounds
	sound_gun_fire[0] = snd_load_wav("data/audio/wp_gun_fire-01.wav");
	sound_gun_fire[0] = snd_load_wav("data/audio/wp_gun_fire-01.wav");
	sound_gun_fire[1] = snd_load_wav("data/audio/wp_gun_fire-02.wav");
	sound_shotty_fire[0] = snd_load_wav("data/audio/wp_shotty_fire-01.wav");
	sound_shotty_fire[1] = snd_load_wav("data/audio/wp_shotty_fire-02.wav");
	sound_shotty_fire[2] = snd_load_wav("data/audio/wp_shotty_fire-03.wav");
	sound_flump_launch[0] = snd_load_wav("data/audio/wp_flump_launch-01.wav");
	sound_flump_launch[1] = snd_load_wav("data/audio/wp_flump_launch-02.wav");
	sound_flump_launch[2] = snd_load_wav("data/audio/wp_flump_launch-03.wav");
	sound_hammer_swing[0] = snd_load_wav("data/audio/wp_hammer_swing-01.wav");
	sound_hammer_swing[1] = snd_load_wav("data/audio/wp_hammer_swing-02.wav");
	sound_hammer_swing[2] = snd_load_wav("data/audio/wp_hammer_swing-03.wav");
	sound_ninja_attack[0] = snd_load_wav("data/audio/wp_ninja_attack-01.wav");
	sound_ninja_attack[1] = snd_load_wav("data/audio/wp_ninja_attack-02.wav");
	sound_ninja_attack[2] = snd_load_wav("data/audio/wp_ninja_attack-03.wav");

	sound_flump_explode[0] = snd_load_wav("data/audio/wp_flump_explo-01.wav");
	sound_flump_explode[1] = snd_load_wav("data/audio/wp_flump_explo-02.wav");
	sound_flump_explode[2] = snd_load_wav("data/audio/wp_flump_explo-03.wav");
	sound_ninja_hit[0] = snd_load_wav("data/audio/wp_ninja_hit-01.wav");
	sound_ninja_hit[1] = snd_load_wav("data/audio/wp_ninja_hit-02.wav");
	sound_ninja_hit[2] = snd_load_wav("data/audio/wp_ninja_hit-03.wav");
	sound_ninja_hit[3] = snd_load_wav("data/audio/wp_ninja_hit-04.wav");

	sound_weapon_switch[0] = snd_load_wav("data/audio/wp_switch-01.wav");
	sound_weapon_switch[1] = snd_load_wav("data/audio/wp_switch-02.wav");
	sound_weapon_switch[2] = snd_load_wav("data/audio/wp_switch-03.wav");

	sound_pain_short[0] = snd_load_wav("data/audio/vo_teefault_pain_short-01.wav");
	sound_pain_short[1] = snd_load_wav("data/audio/vo_teefault_pain_short-02.wav");
	sound_pain_short[2] = snd_load_wav("data/audio/vo_teefault_pain_short-03.wav");
	sound_pain_short[3] = snd_load_wav("data/audio/vo_teefault_pain_short-04.wav");
	sound_pain_short[4] = snd_load_wav("data/audio/vo_teefault_pain_short-05.wav");
	sound_pain_short[5] = snd_load_wav("data/audio/vo_teefault_pain_short-06.wav");
	sound_pain_short[6] = snd_load_wav("data/audio/vo_teefault_pain_short-07.wav");
	sound_pain_short[7] = snd_load_wav("data/audio/vo_teefault_pain_short-08.wav");
	sound_pain_short[8] = snd_load_wav("data/audio/vo_teefault_pain_short-09.wav");
	sound_pain_short[9] = snd_load_wav("data/audio/vo_teefault_pain_short-10.wav");
	sound_pain_short[10] = snd_load_wav("data/audio/vo_teefault_pain_short-11.wav");
	sound_pain_short[11] = snd_load_wav("data/audio/vo_teefault_pain_short-12.wav");

	sound_pain_long[0] = snd_load_wav("data/audio/vo_teefault_pain_long-01.wav");
	sound_pain_long[1] = snd_load_wav("data/audio/vo_teefault_pain_long-02.wav");

	sound_body_land[0] = snd_load_wav("data/audio/foley_land-01.wav");
	sound_body_land[1] = snd_load_wav("data/audio/foley_land-02.wav");
	sound_body_land[2] = snd_load_wav("data/audio/foley_land-03.wav");
	sound_body_land[3] = snd_load_wav("data/audio/foley_land-04.wav");
	sound_body_jump[0] = snd_load_wav("data/audio/foley_foot_left-01.wav");
	sound_body_jump[1] = snd_load_wav("data/audio/foley_foot_left-02.wav");
	sound_body_jump[2] = snd_load_wav("data/audio/foley_foot_left-03.wav");
	sound_body_jump[3] = snd_load_wav("data/audio/foley_foot_left-04.wav");
	sound_body_jump[4] = snd_load_wav("data/audio/foley_foot_right-01.wav");
	sound_body_jump[5] = snd_load_wav("data/audio/foley_foot_right-02.wav");
	sound_body_jump[6] = snd_load_wav("data/audio/foley_foot_right-03.wav");
	sound_body_jump[7] = snd_load_wav("data/audio/foley_foot_right-04.wav");

	sound_body_splat[1] = snd_load_wav("data/audio/foley_body_splat-02.wav");
	sound_body_splat[2] = snd_load_wav("data/audio/foley_body_splat-03.wav");
	sound_body_splat[3] = snd_load_wav("data/audio/foley_body_splat-04.wav");

	sound_spawn[0] = snd_load_wav("data/audio/vo_teefault_spawn-01.wav");
	sound_spawn[1] = snd_load_wav("data/audio/vo_teefault_spawn-02.wav");
	sound_spawn[2] = snd_load_wav("data/audio/vo_teefault_spawn-03.wav");
	sound_spawn[3] = snd_load_wav("data/audio/vo_teefault_spawn-04.wav");
	sound_spawn[4] = snd_load_wav("data/audio/vo_teefault_spawn-05.wav");
	sound_spawn[5] = snd_load_wav("data/audio/vo_teefault_spawn-06.wav");
	sound_spawn[6] = snd_load_wav("data/audio/vo_teefault_spawn-07.wav");

	sound_tee_cry[0] = snd_load_wav("data/audio/vo_teefault_cry-01.wav");
	sound_tee_cry[1] = snd_load_wav("data/audio/vo_teefault_cry-02.wav");

	//sound_hook_loop[0] = snd_load_wav("data/audio/hook_loop-01.wav");
	sound_hook_loop[0] = snd_load_wav("data/audio/hook_loop-02.wav");
	sound_hook_attach[0] = snd_load_wav("data/audio/hook_attach-01.wav");
	sound_hook_attach[1] = snd_load_wav("data/audio/hook_attach-02.wav");
	sound_hook_attach[2] = snd_load_wav("data/audio/hook_attach-03.wav");

	poweruptexcoord[POWERUP_TYPE_HEALTH].x = 10;
	poweruptexcoord[POWERUP_TYPE_HEALTH].y = 2;
	poweruptexcoord[POWERUP_TYPE_HEALTH].w = 2;
	poweruptexcoord[POWERUP_TYPE_HEALTH].h = 2;

	poweruptexcoord[POWERUP_TYPE_ARMOR].x = 12;
	poweruptexcoord[POWERUP_TYPE_ARMOR].y = 2;
	poweruptexcoord[POWERUP_TYPE_ARMOR].w = 2;
	poweruptexcoord[POWERUP_TYPE_ARMOR].h = 2;
	
	poweruptexcoord[POWERUP_TYPE_WEAPON].x = 3;
	poweruptexcoord[POWERUP_TYPE_WEAPON].y = 0;
	poweruptexcoord[POWERUP_TYPE_WEAPON].w = 6;
	poweruptexcoord[POWERUP_TYPE_WEAPON].h = 2;

	poweruptexcoord[POWERUP_TYPE_NINJA].x = 3;
	poweruptexcoord[POWERUP_TYPE_NINJA].y = 10;
	poweruptexcoord[POWERUP_TYPE_NINJA].w = 7;
	poweruptexcoord[POWERUP_TYPE_NINJA].h = 2;

	poweruptexcoord[POWERUP_TYPE_TIMEFIELD].x = 3;
	poweruptexcoord[POWERUP_TYPE_TIMEFIELD].y = 0;
	poweruptexcoord[POWERUP_TYPE_TIMEFIELD].w = 6;
	poweruptexcoord[POWERUP_TYPE_TIMEFIELD].h = 2;

	// Setup weapon cell coords
	float sizemodifier = 1.0f;
	weaponrenderparams[WEAPON_TYPE_GUN].sizex = 60.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_GUN].sizey = 30.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_GUN].offsetx = 32.0f;
	weaponrenderparams[WEAPON_TYPE_GUN].offsety = 4.0f;
	weapontexcoordcursor[WEAPON_TYPE_GUN].x = 0;
	weapontexcoordcursor[WEAPON_TYPE_GUN].y = 4;
	weapontexcoordcursor[WEAPON_TYPE_GUN].w = 2;
	weapontexcoordcursor[WEAPON_TYPE_GUN].h = 2;
	weapontexcoord[WEAPON_TYPE_GUN].x = 2;
	weapontexcoord[WEAPON_TYPE_GUN].y = 4;
	weapontexcoord[WEAPON_TYPE_GUN].w = 4;
	weapontexcoord[WEAPON_TYPE_GUN].h = 2;
	weaponprojtexcoord[WEAPON_TYPE_GUN].x = 6;
	weaponprojtexcoord[WEAPON_TYPE_GUN].y = 4;
	weaponprojtexcoord[WEAPON_TYPE_GUN].w = 2;
	weaponprojtexcoord[WEAPON_TYPE_GUN].h = 2;

	nummuzzletex[WEAPON_TYPE_GUN] = 3;
	muzzletexcoord[WEAPON_TYPE_GUN][0].x = 8;
	muzzletexcoord[WEAPON_TYPE_GUN][0].y = 4;
	muzzletexcoord[WEAPON_TYPE_GUN][0].w = 3;
	muzzletexcoord[WEAPON_TYPE_GUN][0].h = 2;
	muzzletexcoord[WEAPON_TYPE_GUN][1].x = 12;
	muzzletexcoord[WEAPON_TYPE_GUN][1].y = 4;
	muzzletexcoord[WEAPON_TYPE_GUN][1].w = 3;
	muzzletexcoord[WEAPON_TYPE_GUN][1].h = 2;
	muzzletexcoord[WEAPON_TYPE_GUN][2].x = 16;
	muzzletexcoord[WEAPON_TYPE_GUN][2].y = 4;
	muzzletexcoord[WEAPON_TYPE_GUN][2].w = 3;
	muzzletexcoord[WEAPON_TYPE_GUN][2].h = 2;

	muzzleparams[WEAPON_TYPE_GUN].sizex = 60.0f * sizemodifier;
	muzzleparams[WEAPON_TYPE_GUN].sizey = 40.0f * sizemodifier;
	muzzleparams[WEAPON_TYPE_GUN].offsetx = 50.0f * sizemodifier;
	muzzleparams[WEAPON_TYPE_GUN].offsety = 6.0f * sizemodifier;

	sizemodifier = 1.3f;
	weaponrenderparams[WEAPON_TYPE_ROCKET].sizex = 70.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_ROCKET].sizey = 20.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_ROCKET].offsetx = 24.0f;
	weaponrenderparams[WEAPON_TYPE_ROCKET].offsety = -2.0f;
	weapontexcoordcursor[WEAPON_TYPE_ROCKET].x = 0;
	weapontexcoordcursor[WEAPON_TYPE_ROCKET].y = 8;
	weapontexcoordcursor[WEAPON_TYPE_ROCKET].w = 2;
	weapontexcoordcursor[WEAPON_TYPE_ROCKET].h = 2;
	weapontexcoord[WEAPON_TYPE_ROCKET].x = 2;
	weapontexcoord[WEAPON_TYPE_ROCKET].y = 8;
	weapontexcoord[WEAPON_TYPE_ROCKET].w = 7;
	weapontexcoord[WEAPON_TYPE_ROCKET].h = 2;
	weaponprojtexcoord[WEAPON_TYPE_ROCKET].x = 10;
	weaponprojtexcoord[WEAPON_TYPE_ROCKET].y = 8;
	weaponprojtexcoord[WEAPON_TYPE_ROCKET].w = 2;
	weaponprojtexcoord[WEAPON_TYPE_ROCKET].h = 2;

	/*weaponrenderparams[WEAPON_TYPE_SNIPER].sizex = 60.0f;
	weaponrenderparams[WEAPON_TYPE_SNIPER].sizey = 20.0f;
	weaponrenderparams[WEAPON_TYPE_SNIPER].offsetx = 16.0f;
	weaponrenderparams[WEAPON_TYPE_SNIPER].offsety = 4.0f;
	weapontexcoordcursor[WEAPON_TYPE_SNIPER].x = 0;
	weapontexcoordcursor[WEAPON_TYPE_SNIPER].y = 6;
	weapontexcoordcursor[WEAPON_TYPE_SNIPER].w = 2;
	weapontexcoordcursor[WEAPON_TYPE_SNIPER].h = 2;
	weapontexcoord[WEAPON_TYPE_SNIPER].x = 3;
	weapontexcoord[WEAPON_TYPE_SNIPER].y = 6;
	weapontexcoord[WEAPON_TYPE_SNIPER].w = 6;
	weapontexcoord[WEAPON_TYPE_SNIPER].h = 2;
	weaponprojtexcoord[WEAPON_TYPE_SNIPER].x = 10;
	weaponprojtexcoord[WEAPON_TYPE_SNIPER].y = 6;
	weaponprojtexcoord[WEAPON_TYPE_SNIPER].w = 1;
	weaponprojtexcoord[WEAPON_TYPE_SNIPER].h = 1;*/

	weaponrenderparams[WEAPON_TYPE_SHOTGUN].sizex = 80.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_SHOTGUN].sizey = 20.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_SHOTGUN].offsetx = 24.0f;
	weaponrenderparams[WEAPON_TYPE_SHOTGUN].offsety = -2.0f;
	weapontexcoordcursor[WEAPON_TYPE_SHOTGUN].x = 0;
	weapontexcoordcursor[WEAPON_TYPE_SHOTGUN].y = 6;
	weapontexcoordcursor[WEAPON_TYPE_SHOTGUN].w = 2;
	weapontexcoordcursor[WEAPON_TYPE_SHOTGUN].h = 2;
	weapontexcoord[WEAPON_TYPE_SHOTGUN].x = 2;
	weapontexcoord[WEAPON_TYPE_SHOTGUN].y = 6;
	weapontexcoord[WEAPON_TYPE_SHOTGUN].w = 8;
	weapontexcoord[WEAPON_TYPE_SHOTGUN].h = 2;
	weaponprojtexcoord[WEAPON_TYPE_SHOTGUN].x = 10;
	weaponprojtexcoord[WEAPON_TYPE_SHOTGUN].y = 6;
	weaponprojtexcoord[WEAPON_TYPE_SHOTGUN].w = 2;
	weaponprojtexcoord[WEAPON_TYPE_SHOTGUN].h = 2;

	nummuzzletex[WEAPON_TYPE_SHOTGUN] = 3;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][0].x = 12;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][0].y = 6;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][0].w = 3;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][0].h = 2;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][1].x = 16;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][1].y = 6;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][1].w = 3;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][1].h = 2;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][2].x = 20;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][2].y = 6;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][2].w = 3;
	muzzletexcoord[WEAPON_TYPE_SHOTGUN][2].h = 2;

	muzzleparams[WEAPON_TYPE_SHOTGUN].sizex = 60.0f * sizemodifier;
	muzzleparams[WEAPON_TYPE_SHOTGUN].sizey = 40.0f * sizemodifier;
	muzzleparams[WEAPON_TYPE_SHOTGUN].offsetx = 50.0f * sizemodifier;
	muzzleparams[WEAPON_TYPE_SHOTGUN].offsety = 6.0f * sizemodifier;



	weaponrenderparams[WEAPON_TYPE_MELEE].sizex = 60.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_MELEE].sizey = 50.0f * sizemodifier;
	weaponrenderparams[WEAPON_TYPE_MELEE].offsetx = 20.0f;
	weaponrenderparams[WEAPON_TYPE_MELEE].offsety = -4.0f;
	weapontexcoordcursor[WEAPON_TYPE_MELEE].x = 0;
	weapontexcoordcursor[WEAPON_TYPE_MELEE].y = 0;
	weapontexcoordcursor[WEAPON_TYPE_MELEE].w = 2;
	weapontexcoordcursor[WEAPON_TYPE_MELEE].h = 2;
	weapontexcoord[WEAPON_TYPE_MELEE].x = 2;
	weapontexcoord[WEAPON_TYPE_MELEE].y = 1;
	weapontexcoord[WEAPON_TYPE_MELEE].w = 4;
	weapontexcoord[WEAPON_TYPE_MELEE].h = 3;
	weaponprojtexcoord[WEAPON_TYPE_MELEE].x = 0;
	weaponprojtexcoord[WEAPON_TYPE_MELEE].y = 0;
	weaponprojtexcoord[WEAPON_TYPE_MELEE].w = 0;
	weaponprojtexcoord[WEAPON_TYPE_MELEE].h = 0;


	// MODIFIERS
	sizemodifier = 2.0;
	modifierrenderparams[MODIFIER_TYPE_NINJA].sizex = 60.0f * sizemodifier;
	modifierrenderparams[MODIFIER_TYPE_NINJA].sizey = 20.0f * sizemodifier;
	modifierrenderparams[MODIFIER_TYPE_NINJA].offsetx = 20.0f;
	modifierrenderparams[MODIFIER_TYPE_NINJA].offsety = 4.0f;
	modifiertexcoord[MODIFIER_TYPE_NINJA].x = 2;
	modifiertexcoord[MODIFIER_TYPE_NINJA].y = 10;
	modifiertexcoord[MODIFIER_TYPE_NINJA].w = 7;
	modifiertexcoord[MODIFIER_TYPE_NINJA].h = 2;
	modifiertexcoordcursor[MODIFIER_TYPE_NINJA].x = 0;
	modifiertexcoordcursor[MODIFIER_TYPE_NINJA].y = 10;
	modifiertexcoordcursor[MODIFIER_TYPE_NINJA].w = 2;
	modifiertexcoordcursor[MODIFIER_TYPE_NINJA].h = 2;

	modifierrenderparams[MODIFIER_TYPE_TIMEFIELD].sizex = 60.0f * sizemodifier;
	modifierrenderparams[MODIFIER_TYPE_TIMEFIELD].sizey = 20.0f * sizemodifier;
	modifierrenderparams[MODIFIER_TYPE_TIMEFIELD].offsetx = 16.0f;
	modifierrenderparams[MODIFIER_TYPE_TIMEFIELD].offsety = 4.0f;
	modifiertexcoord[MODIFIER_TYPE_TIMEFIELD].x = 0;
	modifiertexcoord[MODIFIER_TYPE_TIMEFIELD].y = 0;
	modifiertexcoord[MODIFIER_TYPE_TIMEFIELD].w = 0;
	modifiertexcoord[MODIFIER_TYPE_TIMEFIELD].h = 0;

	stars[0].x = 0;
	stars[0].y = 0;
	stars[0].w = 2;
	stars[0].h = 2;

	stars[1].x = 0;
	stars[1].y = 2;
	stars[1].w = 2;
	stars[1].h = 2;

	particlecolors[0].x = 0.7f;
	particlecolors[0].y = 0.7f;
	particlecolors[0].z = 0.7f;
	particlecolors[0].w = 1.0f;
	particlestexcoord[0].x = 2;
	particlestexcoord[0].y = 0;
	particlestexcoord[0].w = 2;
	particlestexcoord[0].h = 2;
	particlecolors[1].x = 1.0f;
	particlecolors[1].y = 1.0f;
	particlecolors[1].z = 1.0f; 
	particlecolors[1].w = 1.0f;
	particlestexcoord[1].x = 4;
	particlestexcoord[1].y = 0;
	particlestexcoord[1].w = 2;
	particlestexcoord[1].h = 2;
	particlecolors[2].x = 0.8f;
	particlecolors[2].y = 0.8f;
	particlecolors[2].z = 0.8f;
	particlecolors[2].w = 1.0f;
	particlestexcoord[2].x = 6;
	particlestexcoord[2].y = 0;
	particlestexcoord[2].w = 2;
	particlestexcoord[2].h = 2;
	particlecolors[3].x = 0.988f;
	particlecolors[3].y = 1.0f;
	particlecolors[3].z = 0.16f;
	particlecolors[3].w = 1.0f;
	particlestexcoord[3].x = 8;
	particlestexcoord[3].y = 0;
	particlestexcoord[3].w = 2;
	particlestexcoord[3].h = 2;
	particlecolors[4].x = 1.0f;
	particlecolors[4].y = 1.0f;
	particlecolors[4].z = 1.0f;
	particlecolors[4].w = 1.0f;
	particlestexcoord[4].x = 10;
	particlestexcoord[4].y = 0;
	particlestexcoord[4].w = 2;
	particlestexcoord[4].h = 2;
	particlecolors[5].x = 0.6f;
	particlecolors[5].y = 0.6f;
	particlecolors[5].z = 0.6f;
	particlecolors[5].w = 1.0f;
	particlestexcoord[5].x = 2;
	particlestexcoord[5].y = 2;
	particlestexcoord[5].w = 2;
	particlestexcoord[5].h = 2;
	particlecolors[6].x = 1.0f;
	particlecolors[6].y = 1.0f;
	particlecolors[6].z = 1.0f; 
	particlecolors[6].w = 1.0f;
	particlestexcoord[6].x = 4;
	particlestexcoord[6].y = 2;
	particlestexcoord[6].w = 2;
	particlestexcoord[6].h = 2;
	particlecolors[5].x = 0.9f;
	particlecolors[5].y = 0.9f;
	particlecolors[5].z = 0.9f;
	particlecolors[5].w = 1.0f;
	particlestexcoord[7].x = 6;
	particlestexcoord[7].y = 2;
	particlestexcoord[7].w = 2;
	particlestexcoord[7].h = 2;
	particlecolors[8].x = 1.0f;
	particlecolors[8].y = 1.0f;
	particlecolors[8].z = 1.0f;
	particlecolors[8].w = 1.0f;
	particlestexcoord[8].x = 8;
	particlestexcoord[8].y = 2;
	particlestexcoord[8].w = 2;
	particlestexcoord[8].h = 2;
	lifemodifier[0] = 0.5f;
	lifemodifier[1] = 0.5f;
	lifemodifier[2] = 0.5f;
	lifemodifier[3] = 0.7f;
	lifemodifier[4] = 0.7f;
	lifemodifier[5] = 1.0f;
	lifemodifier[6] = 1.0f;
	lifemodifier[7] = 1.5f;
	lifemodifier[8] = 0.4f;

	chaintexcoord.x = 2;
	chaintexcoord.y = 0;
	chaintexcoord.w = 1;
	chaintexcoord.h = 1;

	chainheadtexcoord.x = 3;
	chainheadtexcoord.y = 0;
	chainheadtexcoord.w = 2;
	chainheadtexcoord.h = 1;


	// anims
	anim::setup_hammer(hammeranim);
	anim::setup_ninja(ninjaanim);

	for (int i = 0; i < NUMHADOKENS; i++)
	{
		hadoken[i].x = 1;
		hadoken[i].y = 12;
		hadoken[i].w = 7;
		hadoken[i].h = 4;
		hadokenparams[i].sizex = 0.0f;
		hadokenparams[i].sizey = 0.0f;
		hadokenparams[i].offsetx = 0.0f;
		hadokenparams[i].offsety = 0.0f;//-hadokenparams[0].sizey * 0.15f;
	}

	// hadoken
	hadoken[0].x = 1;
	hadoken[0].y = 12;
	hadoken[0].w = 7;
	hadoken[0].h = 4;
	hadokenparams[0].sizex = 70.0f * 2.5f;
	hadokenparams[0].sizey = 40.0f * 2.5f;
	hadokenparams[0].offsetx = -60.0f;
	hadokenparams[0].offsety = 0;//-hadokenparams[0].sizey * 0.15f;

	hadoken[2].x = 8;
	hadoken[2].y = 12;
	hadoken[2].w = 8;
	hadoken[2].h = 4;
	hadokenparams[2].sizex = 80.0f * 2.5f;
	hadokenparams[2].sizey = 40.0f * 2.5f;
	hadokenparams[2].offsetx = -60.0f;
	hadokenparams[2].offsety = 0;//-hadokenparams[1].sizey * 0.5f;

	hadoken[4].x = 17;
	hadoken[4].y = 12;
	hadoken[4].w = 7;
	hadoken[4].h = 4;
	hadokenparams[4].sizex = 70.0f * 2.5f;
	hadokenparams[4].sizey = 40.0f * 2.5f;
	hadokenparams[4].offsetx = -60.0f;
	hadokenparams[4].offsety = 0;//-hadokenparams[2].sizey * 0.5f;

	// 0 = outline, 1 = body
	body[0].x = 2;
	body[0].y = 0;
	body[0].w = 2;
	body[0].h = 2;
	body[1].x = 0;
	body[1].y = 0;
	body[1].w = 2;
	body[1].h = 2;

	feet[0].x = 4;
	feet[0].y = 1;
	feet[0].w = 1;
	feet[0].h = 0.5;
	feet[1].x = 4;
	feet[1].y = 1.52;
	feet[1].w = 1;
	feet[1].h = 0.48;
	
	leye.x = 5;
	leye.y = 1;
	leye.w = 0.5;
	leye.h = 0.5;
	reye.x = 5;
	reye.y = 1.5;
	reye.w = 0.5;
	reye.h = 0.5;
}

void modc_entergame()
{
	col_init(32);
	img_init();
	tilemap_init();
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
		
		if(item.type == EVENT_HEALTHMOD)
		{
			ev_healthmod *ev = (ev_healthmod *)data;
			healthmods.create(vec2(ev->x, ev->y), ev->amount);
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
			int sound = (ev->sound & SOUND_MASK);
			bool bstartloop = (ev->sound & SOUND_LOOPFLAG_STARTLOOP) != 0;
			bool bstoploop = (ev->sound & SOUND_LOOPFLAG_STOPLOOP) != 0;
			float vol, pan;
			sound_vol_pan(p, &vol, &pan);

			switch(sound)
			{

			// FIRE!
			case SOUND_FIRE_GUN:
				sound_gun_fire.play_random(volume_gun*vol, pan);
				break;
			case SOUND_FIRE_SHOTGUN:
				sound_shotty_fire.play_random(volume_gun*vol, pan);
				break;
			case SOUND_FIRE_ROCKET:
				sound_flump_launch.play_random(volume_gun*vol, pan);
				break;
			case SOUND_FIRE_MELEE:
				sound_hammer_swing.play_random(volume_gun*vol, pan);
				break;
			case SOUND_FIRE_NINJA:
				sound_ninja_attack.play_random(volume_gun*vol, pan);
				break;

			// IMPACT
			case SOUND_IMPACT_PROJECTILE_GUN:
				break;
			case SOUND_IMPACT_PROJECTILE_SHOTGUN:
				break;
			case SOUND_IMPACT_PROJECTILE_ROCKET:
				sound_flump_explode.play_random(volume_hit*vol, pan);
				break;

			// PLAYER
			case SOUND_PLAYER_JUMP:
				sound_body_jump.play_random(volume_tee*vol, pan);
				break;
			case SOUND_PLAYER_HURT_SHORT:
				sound_pain_short.play_random(volume_tee*vol, pan);
				break;
			case SOUND_PLAYER_HURT_LONG:
				sound_pain_long.play_random(volume_tee*vol, pan);
				break;
			case SOUND_PLAYER_SPAWN:
				sound_spawn.play_random(volume_tee*vol, pan);
				break;
			case SOUND_PLAYER_CHAIN_LOOP:
				sound_hook_loop.play_random(volume_gun*vol, pan);
				break;
			case SOUND_PLAYER_CHAIN_IMPACT:
				sound_hook_attach.play_random(volume_gun*vol, pan);
				break;
			case SOUND_PLAYER_IMPACT:
				sound_body_land.play_random(volume_hit*vol, pan);
				break;
			case SOUND_PLAYER_IMPACT_NINJA:
				sound_ninja_hit.play_random(volume_hit*vol, pan);
				break;
			case SOUND_PLAYER_DIE:
				sound_body_splat.play_random(volume_tee*vol, pan);
				break;
			case SOUND_PLAYER_SWITCHWEAPON:
				sound_weapon_switch.play_random(volume_gun*vol, pan);
				break;
			case SOUND_PLAYER_EQUIP:
				break;
			case SOUND_PLAYER_LAND:
				sound_body_land.play_random(volume_tee*vol, pan);
				break;
			}
		}
	}
}

static void render_projectile(obj_projectile *prev, obj_projectile *current)
{
	gfx_texture_set(texture_weapon);
	gfx_quads_begin();
	cell_select_ex(numcellsx,numcellsy,weaponprojtexcoord[current->type].x, weaponprojtexcoord[current->type].y, weaponprojtexcoord[current->type].w, weaponprojtexcoord[current->type].h);
	vec2 vel(current->vx, current->vy);
	
	// TODO: interpolare angle aswell
	if(length(vel) > 0.00001f)
		gfx_quads_setrotation(get_angle(vel));
	else
		gfx_quads_setrotation(0);
	
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), snap_intratick());
	gfx_quads_draw(pos.x, pos.y,32,32);
	gfx_quads_setrotation(0);
	gfx_quads_end();
}

static void render_powerup(obj_powerup *prev, obj_powerup *current)
{
	//dbg_msg("client", "rendering powerup at %d,%d", current->x, current->y);
	
	gfx_texture_set(texture_weapon);
	gfx_quads_begin();
	float angle = 0.0f;
	float sizex = 64.0f;
	float sizey = 64.0f;
	if (current->type == POWERUP_TYPE_WEAPON)
	{
		angle = -0.25f * pi * 2.0f;
		cell_select_ex(numcellsx,numcellsy,weapontexcoord[current->subtype].x, weapontexcoord[current->subtype].y, weapontexcoord[current->subtype].w, weapontexcoord[current->subtype].h);
		sizex = weaponrenderparams[current->subtype].sizex;
		sizey = weaponrenderparams[current->subtype].sizey;
	}
	else
		cell_select_ex(numcellsx,numcellsy,poweruptexcoord[current->type].x, poweruptexcoord[current->type].y, poweruptexcoord[current->type].w, poweruptexcoord[current->type].h);
	vec2 vel(current->vx, current->vy);
	
	gfx_quads_setrotation(angle);
	// TODO: interpolare angle aswell
	/*if(length(vel) > 0.00001f)
		gfx_quads_setrotation(get_angle(vel));
	else
		gfx_quads_setrotation(0);*/
	
	vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), snap_intratick());
	float offset = pos.y/32.0f + pos.x/32.0f;
	gfx_quads_draw(pos.x+cosf(client_localtime()*2.0f+offset)*2.5f, pos.y+sinf(client_localtime()*2.0f+offset)*2.5f,sizex * 0.65f,sizey * 0.65f);
	gfx_quads_setrotation(0);
	gfx_quads_end();
}

float getmeleeangle(vec2 direction, obj_player* prev, obj_player* player)
{
	vec2 meleedir(0.53, -0.84);
	meleedir = normalize(meleedir);
	vec2 meleedirattack(0.95, -0.3);
	meleedirattack = normalize(meleedirattack);

	if(direction.x < 0)
	{
		meleedir.x = -meleedir.x;
		meleedirattack.x = -meleedirattack.x;
	}

	// 0 -> visualtimeattack go to end pose, (len - visualime) -> go back to normal pose

	float angle = get_angle(meleedir);
	if (prev->attackticks)
	{
		float angleattack = get_angle(meleedirattack);
		int phase1tick = (player->attacklen - player->attackticks);
		if (phase1tick < player->visualtimeattack)
		{
			float intratick = snap_intratick();
			float t = ((((float)phase1tick) + intratick)/(float)player->visualtimeattack);
			angle = LERP(angle, angleattack, min(1.0f,max(0.0f,t)));
		}
		else
		{
			// go back to normal pose
			int phase2tick = (player->attacklen - player->visualtimeattack - player->attackticks);
			float intratick = snap_intratick();
			float t = ((((float)phase2tick) + intratick)/(float)player->visualtimeattack);
			angle = LERP(angleattack, angle, min(1.0f,max(0.0f,t)));
		}
	}
	/*if (prev->attackticks && !player->attackticks)
	{
		// blend back to normal
		float angleattack = get_angle(meleedirattack);
		angle = LERP(angleattack, angle, min(1.0f,max(0.0f,snap_intratick())));
	}
	else if (player->attackticks)
	{
		float angleattack = get_angle(meleedirattack);
		float intratick = snap_intratick();
		float t = ((((float)player->attackticks) - intratick)/(float)player->attacklen);
		angle = LERP(angleattack, angle, min(1.0f,max(0.0f,t)));
	}*/

	return angle;
}

float gethammereangle(vec2 direction, obj_player* prev, obj_player* player)
{
	float t = 0.0f;
	if (prev->attackticks)
		t = 1.0f - ((((float)player->attackticks) - snap_intratick())/(float)player->attacklen);

	vec2 pos;
	float angle = 0.0f;
	hammeranim.evalanim(t,pos,angle);
	if(direction.x < 0)
		angle = pi -angle;// + ;
	//dbg_msg("anim", "Time: %f", t);
	return angle;
}

float getninjaangle(vec2 direction, obj_player* prev, obj_player* player)
{
	float t = 0.0f;
	if (prev->attackticks)
		t = 1.0f - ((((float)player->attackticks) - snap_intratick())/(float)player->attacklen);

	vec2 pos;
	float angle = 0.0f;
	ninjaanim.evalanim(t,pos,angle);
	if(direction.x < 0)
		angle = pi -angle;// + ;
	//dbg_msg("anim", "Time: %f", t);
	return angle;
}


float getrecoil(obj_player* prev, obj_player* player)
{
	// attack = -10
	float recoil = 0.0f;
	if (prev->attackticks)
	{
		float attackrecoil = recoils[player->weapon];
		int phase1tick = (player->attacklen - player->attackticks);
		if (phase1tick < player->visualtimeattack)
		{
			float intratick = snap_intratick();
			float t = ((((float)phase1tick) + intratick)/(float)player->visualtimeattack);
			recoil = LERP(0, attackrecoil, min(1.0f,max(0.0f,t)));
		}
		else
		{
			// go back to normal pose
			int phase2tick = (player->attacklen - player->visualtimeattack - player->attackticks);
			float intratick = snap_intratick();
			float t = ((((float)phase2tick) + intratick)/(float)(player->attacklen - player->visualtimeattack));
			recoil = LERP(attackrecoil, 0.0f, min(1.0f,max(0.0f,t)));
		}
	}
	return recoil;
}

static void render_player(obj_player *prev, obj_player *player)
{
	vec2 direction = get_direction(player->angle);
	float angle = player->angle/256.0f;
	vec2 position = mix(vec2(prev->x, prev->y), vec2(player->x, player->y), snap_intratick());
	
	// draw hook
	if(player->hook_active)
	{
		gfx_texture_set(texture_weapon);
		gfx_quads_begin();
		//gfx_quads_begin();

		vec2 pos = position;
		
		vec2 hook_pos = mix(vec2(prev->hook_x, prev->hook_y), vec2(player->hook_x, player->hook_y), snap_intratick());
		
		float d = distance(pos, hook_pos);
		vec2 dir = normalize(pos-hook_pos);

		gfx_quads_setrotation(get_angle(dir)+pi);
		
		// render head
		cell_select_ex(numcellsx,numcellsy, chainheadtexcoord.x,chainheadtexcoord.y, chainheadtexcoord.w, chainheadtexcoord.h);
		gfx_quads_draw(hook_pos.x, hook_pos.y, 24,16);
		
		// render chain
		cell_select_ex(numcellsx,numcellsy, chaintexcoord.x, chaintexcoord.y, chaintexcoord.w, chaintexcoord.h);
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
		gfx_texture_set(texture_weapon);
		gfx_quads_begin();
		gfx_quads_setrotation(angle);

		if (player->modifier & (1 << MODIFIER_TYPE_NINJA))
		{
			float playerangle = angle;
			// render NINJA!!! (0.53, 0.84) when idle to -> (0.95, 0.3) at the end of attack
			if(direction.x < 0)
				cell_select_ex_flip_y(numcellsx, numcellsy, modifiertexcoord[MODIFIER_TYPE_NINJA].x, modifiertexcoord[MODIFIER_TYPE_NINJA].y, modifiertexcoord[MODIFIER_TYPE_NINJA].w, modifiertexcoord[MODIFIER_TYPE_NINJA].h);
			else
				cell_select_ex(numcellsx, numcellsy, modifiertexcoord[MODIFIER_TYPE_NINJA].x, modifiertexcoord[MODIFIER_TYPE_NINJA].y, modifiertexcoord[MODIFIER_TYPE_NINJA].w, modifiertexcoord[MODIFIER_TYPE_NINJA].h);

			angle = getninjaangle(direction, prev, player);//getmeleeangle(direction, prev, player);
			vec2 ninjadir = get_direction(angle * 256.0f);
			gfx_quads_setrotation(angle);
			vec2 p = position + vec2(0,modifierrenderparams[MODIFIER_TYPE_NINJA].offsety)+ ninjadir * modifierrenderparams[MODIFIER_TYPE_NINJA].offsetx;
			// if attack is active hold it differently and draw speedlines behind us?
			gfx_quads_draw(p.x,p.y/*+bob*/,modifierrenderparams[MODIFIER_TYPE_NINJA].sizex, modifierrenderparams[MODIFIER_TYPE_NINJA].sizey);

			if ((player->attacklen - player->attackticks) <= (SERVER_TICK_SPEED / 5))
			{
				gfx_quads_setrotation(playerangle);
				int ihadoken = rand() % NUMHADOKENS;
				cell_select_ex(numcellsx, numcellsy, hadoken[ihadoken].x, hadoken[ihadoken].y, hadoken[ihadoken].w, hadoken[ihadoken].h);
				vec2 p = position + vec2(0,hadokenparams[ihadoken].offsety)+ direction * hadokenparams[ihadoken].offsetx;
				gfx_quads_draw(p.x,p.y/*+bob*/,hadokenparams[ihadoken].sizex, hadokenparams[ihadoken].sizey);
			}
		}
		else
		{
			// normal weapons
			if(direction.x < 0)
				cell_select_ex_flip_y(numcellsx, numcellsy, weapontexcoord[player->weapon].x, weapontexcoord[player->weapon].y, weapontexcoord[player->weapon].w, weapontexcoord[player->weapon].h);
			else
				cell_select_ex(numcellsx, numcellsy, weapontexcoord[player->weapon].x, weapontexcoord[player->weapon].y, weapontexcoord[player->weapon].w, weapontexcoord[player->weapon].h);

			vec2 dir = direction;
			float recoil = 0.0f;
			if (player->weapon == WEAPON_TYPE_MELEE)
			{
				// if attack is under way, bash stuffs
				//angle = getmeleeangle(direction, prev, player);
				angle = gethammereangle(direction, prev, player);
				gfx_quads_setrotation(angle);
				dir = get_direction(angle * 256.0f);
			}
			else
			{
				recoil = getrecoil(prev, player);
			}

			vec2 p = position + vec2(0,weaponrenderparams[player->weapon].offsety) + dir * weaponrenderparams[player->weapon].offsetx - dir * recoil;
			gfx_quads_draw(p.x,p.y/*+bob*/,weaponrenderparams[player->weapon].sizex, weaponrenderparams[player->weapon].sizey);
			// draw muzzleflare
			if (player->weapon == WEAPON_TYPE_GUN || player->weapon == WEAPON_TYPE_SHOTGUN)
			{
				// check if we're firing stuff
				if (true)///prev->attackticks)
				{
					float alpha = 0.0f;
					int phase1tick = (player->attacklen - player->attackticks);
					if (phase1tick < (player->visualtimeattack + 3))
					{
						float intratick = snap_intratick();
						float t = ((((float)phase1tick) + intratick)/(float)player->visualtimeattack);
						alpha = LERP(2.0, 0.0f, min(1.0f,max(0.0f,t)));
					}

					if (alpha > 0.0f)
					{
						float offsety = -muzzleparams[player->weapon].offsety;
						int itex = rand() % nummuzzletex[player->weapon];
						if(direction.x < 0)
						{
							offsety = -offsety;
							cell_select_ex_flip_y(numcellsx, numcellsy, muzzletexcoord[player->weapon][itex].x, muzzletexcoord[player->weapon][itex].y, muzzletexcoord[player->weapon][itex].w, muzzletexcoord[player->weapon][itex].h);
						}
						else
							cell_select_ex(numcellsx, numcellsy, muzzletexcoord[player->weapon][itex].x, muzzletexcoord[player->weapon][itex].y, muzzletexcoord[player->weapon][itex].w, muzzletexcoord[player->weapon][itex].h);

						gfx_quads_setcolor(1.0f,1.0f,1.0f,alpha);
						vec2 diry(-dir.y,dir.x);
						p += dir * muzzleparams[player->weapon].offsetx + diry * offsety;
						gfx_quads_draw(p.x,p.y/*+bob*/,muzzleparams[player->weapon].sizex, muzzleparams[player->weapon].sizey);
					}
				}
			}
		}
		/*else
		{
			// minigun
			if(direction.x < 0)
				cell_select_flip_y(4,4,8,2);
			else
				cell_select(4,4,8,2);
			vec2 p = position + vec2(0,3);
			gfx_quads_draw(p.x,p.y,8*8,8*2);
		}*/
			
		gfx_quads_setrotation(0);
		gfx_quads_end();
	}
	
	
	gfx_texture_set(texture_char_default);
	gfx_quads_begin();
	
	float bob = 0;
	
	// draw foots
	const float cyclelength = 128.0f;
	const float steplength = 26;
	const float lift = 4.0f;
	bool stationary = player->vx < 1 && player->vx > -1;
	bool inair = col_check_point(player->x, player->y+16) == 0;
	
	for(int p = 0; p < 2; p++)
	{
		// first pass we draw the outline
		// second pass we draw the filling
		
		//int v_offset = p?0:5;
		int outline = p;// ? 1 : 0;
		float offsety = charids[player->clientid % 16] * 2.0f;
		
		for(int f = 0; f < 2; f++)
		{
			float basesize = 10.0f;
			if(f == 1)
			{
				// draw body
				float t = fmod(position.x, cyclelength/2)/(cyclelength/2);
				bob = -sinf(pow(t,2)*pi) * 3;
				cell_select_ex(charnumcellsx,charnumcellsy, body[outline].x,body[outline].y + offsety,body[outline].w,body[outline].h);
				//cell_select_ex(16,16, 0,0+v_offset,4,4);
				//const float size = 64.0f;
				if(stationary || inair)
					bob = 0;
				gfx_quads_draw(position.x, position.y-5+bob, 4*basesize, 4*basesize);
				
				// draw eyes
				if(p == 1)
				{
					//cell_select_ex(16,16, 8,3,1,1);
					vec2 md = get_direction(player->angle);
					float mouse_dir_x = md.x;
					float mouse_dir_y = md.y;
					
					// normal
					cell_select_ex(charnumcellsx,charnumcellsy, leye.x,leye.y + offsety,leye.w,leye.h);
					gfx_quads_draw(position.x-4+mouse_dir_x*4, position.y-8+mouse_dir_y*3+bob, basesize, basesize);
					cell_select_ex(charnumcellsx,charnumcellsy, reye.x,reye.y + offsety,reye.w,reye.h);
					gfx_quads_draw(position.x+4+mouse_dir_x*4, position.y-8+mouse_dir_y*3+bob, basesize, basesize);
				}
			}

			// draw feet
			//cell_select_ex(16,16, 5,2+v_offset, 2,2);
			cell_select_ex(charnumcellsx,charnumcellsy, feet[outline].x,feet[outline].y + offsety, feet[outline].w,feet[outline].h);
			float w = basesize*2.5f;
			float h = basesize*1.425f;
			if(inair)
			{
				float r = 0.0f;
				if(player->vy < 0.0f)
					r = player->vy/3.0f;
				else
					r = player->vy/15.0f;

				// clamp the rotation
				if(r > 0.5f) r = 0.5f;
				if(r < -0.5f) r = -0.5f;
				
				if(player->vx > 0.0f)
					r *= -1.0f;
				gfx_quads_setrotation(r);
				gfx_quads_drawTL(position.x-4+f*7-w/2, position.y+16 - h, w, h);
				gfx_quads_setrotation(0);
			}
			else if(stationary)
			{
				// stationary
				gfx_quads_drawTL(position.x-7+f*14-w/2, position.y+16 - h, w, h);
			}
			else
			{
				/*
					The walk cycle, 2 parts
					
						  111  
						 1   1   
						2     1
						 2     1
						  2222221 
					GROUND GROUND GROUND
				*/
				
				// moving
				float tx = position.x+f*(cyclelength/2);
				float t = fmod(tx, cyclelength) / cyclelength;
				if(player->vx < 0)
					t = 1.0f-t;
				
				float y;
				float x = 0;
				float r = 0;
				float r_back = 1.5f;
				
				if(t < 0.5f)
				{
					// stomp down foot (part 1)
					float st = t*2;
					y = 1.0f-pow(st, 0.5f) + sinf(pow(st,2)*pi)*0.5f;
					x = -steplength/2 + st*steplength;
					r = r_back*(1-st) + sinf(pow(st,1.5f)*pi*2);
				}
				else
				{
					// lift foot up again (part 2)
					float st = (t-0.5f)*2;
					y = pow(st, 5.0f);
					x = steplength/2 - st*steplength;
					r = y*r_back;
				}
				

				if(player->vx > 0)
				{
					gfx_quads_setrotation(r);
					gfx_quads_drawTL(position.x+x-w/2, position.y+16-y*lift - h, w, h);
				}
				else
				{
					gfx_quads_setrotation(-r);
					gfx_quads_drawTL(position.x-x-w/2, position.y+16-y*lift - h, w, h);
				}
				gfx_quads_setrotation(0);
			}

		}
	}
	
	gfx_quads_end();
	
	
}

static player_input oldinput;
static bool bfirst = true;
void modc_render()
{	
	if (bfirst)
	{
		bfirst = false;
		oldinput.activeweapon = 0;
		oldinput.angle = 0;
		oldinput.blink = 0;
		oldinput.fire = 0;
		oldinput.hook = 0;
		oldinput.jump = 0;
		oldinput.left = 0;
		oldinput.right = 0;
	}
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
		input.left = inp_key_pressed('A');
		input.right = inp_key_pressed('D');
		float a = atan((float)mouse_pos.y/(float)mouse_pos.x);
		if(mouse_pos.x < 0)
			a = a+pi;
		input.angle = (int)(a*256.0f);
		input.jump = inp_key_pressed(baselib::keys::space) || inp_key_pressed('W');
		
		input.fire = inp_mouse_button_pressed(0);// | (oldinput.fire << 16);
		//oldinput.fire = input.fire & 0x0000ffff;
		
		input.hook = inp_mouse_button_pressed(1) || inp_key_pressed(baselib::keys::lctrl); // be nice to mac users O.o
		input.blink = inp_key_pressed('S');
		
		// Weapon switching
		input.activeweapon = inp_key_pressed('1') ? 0x80000000 : 0;
		if (!input.activeweapon)
			input.activeweapon = inp_key_pressed('2') ? 0x80000000 | 1 : 0;
		if (!input.activeweapon)
			input.activeweapon = inp_key_pressed('3') ? 0x80000000 | 2 : 0;
		if (!input.activeweapon)
			input.activeweapon = inp_key_pressed('4') ? 0x80000000 | 3 : 0;
		/*if (!input.activeweapon)
			input.activeweapon = inp_key_pressed('5') ? 0x80000000 | 4 : 0;*/

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
						local_player_pos = mix(vec2(((obj_player *)p)->x, ((obj_player *)p)->y), local_player_pos, snap_intratick());
					break;
				}
			}
		}
	}

	// pseudo format
	float zoom = inp_key_pressed('T') ? 1.0 : 3.0f;
	
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
			
			//gfx_quads_draw_freeform(0,0, -100,0, -100,-100, 0,-100);
			
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
		
		gfx_texture_set(texture_sun);
		gfx_quads_begin();
		gfx_quads_draw(pos.x, pos.y, 256, 256);
		gfx_quads_end();
	}
	
	// render map
	tilemap_render(32.0f, 0);
#ifdef _DEBUG
	float speed = 0.0f;
#endif
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
			{
				render_player((obj_player *)prev, (obj_player *)data);
/*#ifdef _DEBUG
				{
					obj_player *p = (obj_player *)prev;
					obj_player *c = (obj_player *)data;
					vec2 positionold = vec2(p->x, p->y);
					vec2 poscur = vec2(c->x, c->y);
					speed = distance(positionold,poscur);
				}
#endif*/
			}
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
	
	// render health mods
	healthmods.render();
	
	// render cursor 
	// FIXME CURSOR!!!
	
	if(local_player)
	{
		gfx_texture_set(texture_weapon);
		gfx_quads_begin();
		if (local_player->modifier & (1 << MODIFIER_TYPE_NINJA))
			cell_select_ex(numcellsx,numcellsy, modifiertexcoordcursor[MODIFIER_TYPE_NINJA].x, modifiertexcoordcursor[MODIFIER_TYPE_NINJA].y, modifiertexcoordcursor[MODIFIER_TYPE_NINJA].w, modifiertexcoordcursor[MODIFIER_TYPE_NINJA].h);
		else
			cell_select_ex(numcellsx,numcellsy, weapontexcoordcursor[local_player->weapon].x, weapontexcoordcursor[local_player->weapon].y, weapontexcoordcursor[local_player->weapon].w, weapontexcoordcursor[local_player->weapon].h);
		float cursorsize = 64;
		gfx_quads_draw(local_player_pos.x+mouse_pos.x, local_player_pos.y+mouse_pos.y,cursorsize,cursorsize);


		// render ammo count
		// render gui stuff
		gfx_quads_end();
		gfx_quads_begin();
		gfx_mapscreen(0,0,400,300);
		cell_select_ex(numcellsx,numcellsy, weaponprojtexcoord[local_player->weapon].x, weaponprojtexcoord[local_player->weapon].y, weaponprojtexcoord[local_player->weapon].w, weaponprojtexcoord[local_player->weapon].h);
		for (int i = 0; i < local_player->ammocount; i++)
		{
			gfx_quads_drawTL(10+i*12,34,10,10);
		}
		gfx_quads_end();

		gfx_texture_set(texture_game);
		gfx_quads_begin();
		int h = 0;
		cell_select_ex(32,16, 0,0, 4,4);
		for(; h < local_player->health; h++)
			gfx_quads_drawTL(10+h*12,10,10,10);
		
		cell_select_ex(32,16, 5,0, 4,4);
		for(; h < 10; h++)
			gfx_quads_drawTL(10+h*12,10,10,10);

		h = 0;
		cell_select_ex(32,16, 0,5, 4,4);
		for(; h < local_player->armor; h++)
			gfx_quads_drawTL(10+h*12,22,10,10);
		
		cell_select_ex(32,16, 5,5, 4,4);
		for(; h < 10; h++)
			gfx_quads_drawTL(10+h*12,22,10,10);
		gfx_quads_end();

		// render speed
/*#ifdef _DEBUG
		gfx_texture_set(font_texture);
		char text[256];
		sprintf(text,"speed: %f",speed);
		gfx_quads_text(300,20,10,text);
#endif*/
	}
	// render gui stuff
	gfx_mapscreen(0,0,400,300);
	// render score board
	if(inp_key_pressed(baselib::keys::tab))
	{
		gfx_texture_set(font_texture);
		gfx_quads_text(10, 50, 8, "Score Board");

		int num = snap_num_items(SNAP_CURRENT);
		int row = 1;
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
					char name[32];
					snap_decode_string(player->name, name, 32);
					sprintf(buf, "%4d %s", player->score, name);
					gfx_quads_text(10, 50 + 10 * row, 8, buf);
					row++;
				}
			}
		}
	}
}
