#include "../../engine/interface.h"
#include "mapres_tilemap.h"
#include "mapres_image.h"
#include "../mapres.h"
#include "../../engine/config.h"

#include <baselib/opengl.h>

void *batches[32] = {0};

int tilemap_init()
{
	for(int i = 0; i < 32; i++)
		if(batches[i])
		{
			gfx_destoy_batch(batches[i]);
			batches[i] = 0;
		}
	return 0;
}

void tilemap_render(float scale, int fg)
{
	if(!map_is_loaded())
		return;
	
	float screen_x0, screen_y0, screen_x1, screen_y1;
	gfx_getscreen(&screen_x0, &screen_y0, &screen_x1, &screen_y1);
	
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
			if(!config.gfx_high_detail && !tmap->main)
				continue;
			gfx_texture_set(img_get(tmap->image));
			
			if(!batches[t])
			{
				gfx_quads_begin();
				
				int starty = (int)(screen_y0/scale)-1;
				int startx = (int)(screen_x0/scale)-1;
				int endy = (int)(screen_y1/scale)+1;
				int endx = (int)(screen_x1/scale)+1;
				
				float frac = (1.0f/1024.0f);//2.0f; //2.0f;
				float texsize = 1024.0f;
				float nudge = 0.5f/texsize;
				int border = 24;
				for(int y = starty; y < endy; y++)
					for(int x = startx; x < endx; x++)
					{
						int mx = x;
						int my = y;
						if(mx<0) mx = 0;
						if(mx>=tmap->width) mx = tmap->width-1;
						if(my<0) my = 0;
						if(my>=tmap->height) my = tmap->height-1;
						
						int c = mx + my*tmap->width;
							
						unsigned char d = data[c*2];
						if(d)
						{
							/*
							gfx_quads_setsubset(
								(d%16)/16.0f*s+frac,
								(d/16)/16.0f*s+frac,
								((d%16)/16.0f+1.0f/16.0f)*s-frac,
								((d/16)/16.0f+1.0f/16.0f)*s-frac);
								*/
							
							int tx = d%16;
							int ty = d/16;
							int px0 = tx*(1024/16);
							int py0 = ty*(1024/16);
							int px1 = (tx+1)*(1024/16)-1;
							int py1 = (ty+1)*(1024/16)-1;

							gfx_quads_setsubset(
								nudge + px0/texsize+frac,
								nudge + py0/texsize+frac,
								nudge + px1/texsize-frac,
								nudge + py1/texsize-frac);

							gfx_quads_drawTL(x*scale, y*scale, scale, scale);
						}
					}
				
				gfx_quads_end();
				//batches[t] = gfx_quads_create_batch();
			}
			
			//gfx_quads_draw_batch(batches[t]);
			//glCallList(lists_start+t);
		}
	}
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
}
