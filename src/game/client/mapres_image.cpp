#include <baselib/system.h>
#include "../../engine/interface.h"
#include "mapres_image.h"
#include "../mapres.h"

static int map_textures[64] = {0};
static int count = 0;

int img_init()
{
	int start, count;
	map_get_type(MAPRES_IMAGE, &start, &count);
	dbg_msg("mapres_image", "start=%d count=%d", start, count);
	for(int i = 0; i < 64; i++)
	{
		if(map_textures[i])
		{
			gfx_unload_texture(map_textures[i]);
			map_textures[i] = 0;
		}
	}

	for(int i = 0; i < count; i++)
	{
		mapres_image *img = (mapres_image *)map_get_item(start+i, 0, 0);
		void *data = map_get_data(img->image_data);
		map_textures[i] = gfx_load_texture_raw(img->width, img->height, IMG_RGBA, data);
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
