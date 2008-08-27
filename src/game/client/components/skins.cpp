/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <base/system.h>
#include <base/math.hpp>

#include <engine/e_client_interface.h>
#include "skins.hpp"

SKINS::SKINS()
{
	num_skins = 0;
}

void SKINS::skinscan(const char *name, int is_dir, void *user)
{
	SKINS *self = (SKINS *)user;
	int l = strlen(name);
	if(l < 4 || is_dir || self->num_skins == MAX_SKINS)
		return;
	if(strcmp(name+l-4, ".png") != 0)
		return;
		
	char buf[512];
	str_format(buf, sizeof(buf), "data/skins/%s", name);
	IMAGE_INFO info;
	if(!gfx_load_png(&info, buf))
	{
		dbg_msg("game", "failed to load skin from %s", name);
		return;
	}
	
	self->skins[self->num_skins].org_texture = gfx_load_texture_raw(info.width, info.height, info.format, info.data, info.format, 0);
	
	int body_size = 96; // body size
	unsigned char *d = (unsigned char *)info.data;
	int pitch = info.width*4;

	// dig out blood color
	{
		int colors[3] = {0};
		for(int y = 0; y < body_size; y++)
			for(int x = 0; x < body_size; x++)
			{
				if(d[y*pitch+x*4+3] > 128)
				{
					colors[0] += d[y*pitch+x*4+0];
					colors[1] += d[y*pitch+x*4+1];
					colors[2] += d[y*pitch+x*4+2];
				}
			}
			
		self->skins[self->num_skins].blood_color = normalize(vec3(colors[0], colors[1], colors[2]));
	}
	
	// create colorless version
	int step = info.format == IMG_RGBA ? 4 : 3;

	// make the texture gray scale
	for(int i = 0; i < info.width*info.height; i++)
	{
		int v = (d[i*step]+d[i*step+1]+d[i*step+2])/3;
		d[i*step] = v;
		d[i*step+1] = v;
		d[i*step+2] = v;
	}

	
	if(1)
	{
		int freq[256] = {0};
		int org_weight = 0;
		int new_weight = 192;
		
		// find most common frequence
		for(int y = 0; y < body_size; y++)
			for(int x = 0; x < body_size; x++)
			{
				if(d[y*pitch+x*4+3] > 128)
					freq[d[y*pitch+x*4]]++;
			}
		
		for(int i = 1; i < 256; i++)
		{
			if(freq[org_weight] < freq[i])
				org_weight = i;
		}

		// reorder
		int inv_org_weight = 255-org_weight;
		int inv_new_weight = 255-new_weight;
		for(int y = 0; y < body_size; y++)
			for(int x = 0; x < body_size; x++)
			{
				int v = d[y*pitch+x*4];
				if(v <= org_weight)
					v = (int)(((v/(float)org_weight) * new_weight));
				else
					v = (int)(((v-org_weight)/(float)inv_org_weight)*inv_new_weight + new_weight);
				d[y*pitch+x*4] = v;
				d[y*pitch+x*4+1] = v;
				d[y*pitch+x*4+2] = v;
			}
	}
	
	self->skins[self->num_skins].color_texture = gfx_load_texture_raw(info.width, info.height, info.format, info.data, info.format, 0);
	mem_free(info.data);

	// set skin data	
	strncpy(self->skins[self->num_skins].name, name, min((int)sizeof(self->skins[self->num_skins].name),l-4));
	dbg_msg("game", "load skin %s", self->skins[self->num_skins].name);
	self->num_skins++;
}


void SKINS::init()
{
	// load skins
	num_skins = 0;
	fs_listdir("data/skins", skinscan, this);
}

int SKINS::num()
{
	return num_skins;	
}

const SKINS::SKIN *SKINS::get(int index)
{
	return &skins[index%num_skins];
}

int SKINS::find(const char *name)
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

static vec3 hsl_to_rgb(vec3 in)
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

vec4 SKINS::get_color(int v)
{
	vec3 r = hsl_to_rgb(vec3((v>>16)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
	return vec4(r.r, r.g, r.b, 1.0f);
}
