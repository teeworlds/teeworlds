#include <string.h>
#include <stdio.h>
#include <engine/system.h>
#include <engine/interface.h>
#include "cl_skin.h"
#include "../math.h"

enum
{
	MAX_SKINS=256,
};

static skin skins[MAX_SKINS] = {{-1, -1, {0}, {0}}};
static int num_skins = 0;

static void skinscan(const char *name, int is_dir, void *user)
{
	int l = strlen(name);
	if(l < 4 || is_dir || num_skins == MAX_SKINS)
		return;
	if(strcmp(name+l-4, ".png") != 0)
		return;
		
	char buf[512];
	sprintf(buf, "data/skins/%s", name);
	IMAGE_INFO info;
	if(!gfx_load_png(&info, buf))
	{
		dbg_msg("game", "failed to load skin from %s", name);
		return;
	}
	
	skins[num_skins].org_texture = gfx_load_texture_raw(info.width, info.height, info.format, info.data);
	
	// create colorless version
	unsigned char *d = (unsigned char *)info.data;
	int step = info.format == IMG_RGBA ? 4 : 3;
	
	for(int i = 0; i < info.width*info.height; i++)
	{
		int v = (d[i*step]+d[i*step+1]+d[i*step+2])/3;
		d[i*step] = v;
		d[i*step+1] = v;
		d[i*step+2] = v;
	}
	
	skins[num_skins].color_texture = gfx_load_texture_raw(info.width, info.height, info.format, info.data);
	mem_free(info.data);

	// set skin data	
	strncpy(skins[num_skins].name, name, min((int)sizeof(skins[num_skins].name),l-4));
	dbg_msg("game", "load skin %s", skins[num_skins].name);
	num_skins++;
}


void skin_init()
{
	// load skins
	fs_listdir("data/skins", skinscan, 0);
}

int skin_num()
{
	return num_skins;	
}

const skin *skin_get(int index)
{
	return &skins[index%num_skins];
}

int skin_find(const char *name)
{
	for(int i = 0; i < num_skins; i++)
	{
		if(strcmp(skins[i].name, name) == 0)
			return i;
	}
	return -1;
}




// these converter functions were nicked from some random internet pages
static float hue_to_rgb(float v1, float v2, float h)
{
   if(h < 0) h += 1;
   if(h > 1) h -= 1;
   if((6 * h) < 1) return v1 + ( v2 - v1 ) * 6 * h;
   if((2 * h) < 1) return v2;
   if((3 * h) < 2) return v1 + ( v2 - v1 ) * ((2.0f/3.0f) - h) * 6;
   return v1;
}

vec3 hsl_to_rgb(vec3 in)
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

vec4 skin_get_color(int v)
{
	vec3 r = hsl_to_rgb(vec3((v>>16)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
	return vec4(r.r, r.g, r.b, 1.0f);
}
