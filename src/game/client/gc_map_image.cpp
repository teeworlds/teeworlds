/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>

#include <stdio.h>
#include <engine/e_client_interface.h>
#include <game/g_mapitems.hpp>
#include "gc_map_image.hpp"

static int map_textures[64] = {0};
static int count = 0;
/*
static void calc_mipmaps(void *data_in, unsigned width, unsigned height, void *data_out)
{
	unsigned char *src = (unsigned char*)data_in;
	unsigned char *dst = (unsigned char*)data_out;
	unsigned mip_w = width;
	unsigned mip_h = height;
	unsigned prev_w;
	unsigned prev_h;

	// Highest level - no mod
	for(unsigned x = 0; x < mip_w; x++)
	{
		for(unsigned y = 0; y < mip_h; y++)
		{
			unsigned i = (y * mip_w + x)<<2;
			for(unsigned j = 0; j < 4; j++)
				dst[i+j] = src[i+j];
		}
	}

	src = dst;
	dst += mip_w * mip_h * 4;
	prev_w = mip_w;
	prev_h = mip_h;
	mip_w = mip_w>>1;
	mip_h = mip_h>>1;

	while(mip_w > 0 && mip_h > 0)
	{
		for(unsigned x = 0; x < mip_w; x++)
		{
			for(unsigned y = 0; y < mip_h; y++)
			{
				unsigned i = (y * mip_w + x)<<2;

				unsigned r = 0;
				unsigned g = 0;
				unsigned b = 0;
				unsigned a = 0;


				r += src[(((y<<1) * prev_w + (x<<1))<<2)];
				g += src[(((y<<1) * prev_w + (x<<1))<<2)+1];
				b += src[(((y<<1) * prev_w + (x<<1))<<2)+2];
				a += src[(((y<<1) * prev_w + (x<<1))<<2)+3];

				r += src[(((y<<1) * prev_w + ((x+1)<<1))<<2)];
				g += src[(((y<<1) * prev_w + ((x+1)<<1))<<2)+1];
				b += src[(((y<<1) * prev_w + ((x+1)<<1))<<2)+2];
				a += src[(((y<<1) * prev_w + ((x+1)<<1))<<2)+3];

				r += src[((((y+1)<<1) * prev_w + (x<<1))<<2)];
				g += src[((((y+1)<<1) * prev_w + (x<<1))<<2)+1];
				b += src[((((y+1)<<1) * prev_w + (x<<1))<<2)+2];
				a += src[((((y+1)<<1) * prev_w + (x<<1))<<2)+3];

				r += src[((((y+1)<<1) * prev_w + ((x+1)<<1))<<2)];
				g += src[((((y+1)<<1) * prev_w + ((x+1)<<1))<<2)+1];
				b += src[((((y+1)<<1) * prev_w + ((x+1)<<1))<<2)+2];
				a += src[((((y+1)<<1) * prev_w + ((x+1)<<1))<<2)+3];

				dst[i]   = r>>2;
				dst[i+1] = g>>2;
				dst[i+2] = b>>2;
				dst[i+3] = a>>2;
			}
		}

		src = dst;
		dst = dst + mip_w*mip_h*4;
		prev_w = mip_w;
		prev_h = mip_h;
		mip_w = mip_w>>1;
		mip_h = mip_h>>1;
	}
}
extern int DEBUGTEST_MAPIMAGE;
*/


int img_init()
{
	int start, count;
	map_get_type(MAPITEMTYPE_IMAGE, &start, &count);
	dbg_msg("image", "start=%d count=%d", start, count);
	for(int i = 0; i < 64; i++)
	{
		if(map_textures[i])
		{
			gfx_unload_texture(map_textures[i]);
			map_textures[i] = -1;
		}
	}

	//void *data_res = (void*)mem_alloc(1024*1024*4*2, 16);
	for(int i = 0; i < count; i++)
	{
		MAPITEM_IMAGE *img = (MAPITEM_IMAGE *)map_get_item(start+i, 0, 0);
		if(img->external)
		{
			char buf[256];
			char *name = (char *)map_get_data(img->image_name);
			str_format(buf, sizeof(buf), "data/mapres/%s.png", name);
			map_textures[i] = gfx_load_texture(buf, IMG_AUTO, 0);
		}
		else
		{
			void *data = map_get_data(img->image_data);
			map_textures[i] = gfx_load_texture_raw(img->width, img->height, IMG_RGBA, data, IMG_RGBA, 0);
			map_unload_data(img->image_data);
		}
	}

	return count;
}

int img_num()
{
	return count;
}

int img_get(int index)
{
	return map_textures[index];
}
