#include "../interface.h"
#include "mapres_tilemap.h"
#include "mapres_image.h"
#include "mapres.h"

int tilemap_init()
{
	return 0;
}

void tilemap_render(float scale, int fg)
{
	if(!map_is_loaded())
		return;
	
	// fetch indecies
	int start, num;
	map_get_type(MAPRES_TILEMAP, &start, &num);

	// render tilemaps
	int passed_main = 0;
	for(int t = 0; t < num; t++)
	{
		mapres_tilemap *tmap = (mapres_tilemap *)map_get_item(start+t,0,0);
		unsigned char *data = (unsigned char *)map_get_data(tmap->data);
		
		if(tmap->main)
			passed_main = 1;

		if((fg && passed_main) || (!fg && !passed_main))
		{
			gfx_texture_set(img_get(tmap->image));
			gfx_quads_begin();
			
			int c = 0;
			float frac = (1.0f/1024.0f); //2.0f;
			for(int y = 0; y < tmap->height; y++)
				for(int x = 0; x < tmap->width; x++, c++)
				{
					unsigned char d = data[c*2];
					if(d)
					{
						gfx_quads_setsubset(
							(d%16)/16.0f+frac,
							(d/16)/16.0f+frac,
							(d%16)/16.0f+1.0f/16.0f-frac,
							(d/16)/16.0f+1.0f/16.0f-frac);
						gfx_quads_drawTL(x*scale, y*scale, scale, scale);
					}
				}
			gfx_quads_end();
		}
	}
}
