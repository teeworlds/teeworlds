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

