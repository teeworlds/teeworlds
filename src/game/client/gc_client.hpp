#include <base/vmath.hpp>

#include <game/generated/g_protocol.hpp>
#include <game/gamecore.hpp>

#include <game/client/gc_render.hpp>

extern TUNING_PARAMS tuning;

// predicted players
extern CHARACTER_CORE predicted_prev_char;
extern CHARACTER_CORE predicted_char;

// extra projs
enum
{
	MAX_EXTRA_PROJECTILES=32,
};

extern NETOBJ_PROJECTILE extraproj_projectiles[MAX_EXTRA_PROJECTILES];
extern int extraproj_num;
void extraproj_reset();

// various helpers
//void snd_play_random(int chn, int setid, float vol, vec2 pos);
//void chat_enable_mode(int team);

inline vec2 random_dir() { return normalize(vec2(frandom()-0.5f, frandom()-0.5f)); }

inline float hue_to_rgb(float v1, float v2, float h)
{
   if(h < 0) h += 1;
   if(h > 1) h -= 1;
   if((6 * h) < 1) return v1 + ( v2 - v1 ) * 6 * h;
   if((2 * h) < 1) return v2;
   if((3 * h) < 2) return v1 + ( v2 - v1 ) * ((2.0f/3.0f) - h) * 6;
   return v1;
}

inline vec3 hsl_to_rgb(vec3 in)
{
	float v1, v2;
	vec3 out;

	if(in.s == 0)
	{
		out.r = in.l;
		out.g = in.l;
		out.b = in.l;
	}
	else
	{
		if(in.l < 0.5f) 
			v2 = in.l * (1 + in.s);
		else           
			v2 = (in.l+in.s) - (in.s*in.l);

		v1 = 2 * in.l - v2;

		out.r = hue_to_rgb(v1, v2, in.h + (1.0f/3.0f));
		out.g = hue_to_rgb(v1, v2, in.h);
		out.b = hue_to_rgb(v1, v2, in.h - (1.0f/3.0f));
	} 

	return out;
}
