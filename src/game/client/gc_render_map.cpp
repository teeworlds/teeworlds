/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_interface.h>
#include "../g_math.h"
#include "gc_client.h"

static void rotate(POINT *center, POINT *point, float rotation)
{
	int x = point->x - center->x;
	int y = point->y - center->y;
	point->x = (int)(x * cosf(rotation) - y * sinf(rotation) + center->x);
	point->y = (int)(x * sinf(rotation) + y * cosf(rotation) + center->y);
}

void render_quads(QUAD *quads, int num_quads)
{
	gfx_quads_begin();
	float conv = 1/255.0f;
	for(int i = 0; i < num_quads; i++)
	{
		QUAD *q = &quads[i];
		
		gfx_quads_setsubset_free(
			fx2f(q->texcoords[0].x), fx2f(q->texcoords[0].y),
			fx2f(q->texcoords[1].x), fx2f(q->texcoords[1].y),
			fx2f(q->texcoords[2].x), fx2f(q->texcoords[2].y),
			fx2f(q->texcoords[3].x), fx2f(q->texcoords[3].y)
		);

		float r=1, g=1, b=1, a=1;
		float offset_x = 0;
		float offset_y = 0;
		float rot = 0;
		
		/*
		// TODO: fix this
		if(editor.animate)
		{
			if(q->pos_env >= 0 && q->pos_env < editor.map.envelopes.len())
			{
				ENVELOPE *e = editor.map.envelopes[q->pos_env];
				float t = editor.animate_time+q->pos_env_offset/1000.0f;
				offset_x = e->eval(t, 0);
				offset_y = e->eval(t, 1);
				rot = e->eval(t, 2);
			}
			
			if(q->color_env >= 0 && q->color_env < editor.map.envelopes.len())
			{
				ENVELOPE *e = editor.map.envelopes[q->color_env];
				float t = editor.animate_time+q->color_env_offset/1000.0f;
				r = e->eval(t, 0);
				g = e->eval(t, 1);
				b = e->eval(t, 2);
				a = e->eval(t, 3);
			}
		}*/
		
		gfx_setcolorvertex(0, q->colors[0].r*conv*r, q->colors[0].g*conv*g, q->colors[0].b*conv*b, q->colors[0].a*conv*a);
		gfx_setcolorvertex(1, q->colors[1].r*conv*r, q->colors[1].g*conv*g, q->colors[1].b*conv*b, q->colors[1].a*conv*a);
		gfx_setcolorvertex(2, q->colors[2].r*conv*r, q->colors[2].g*conv*g, q->colors[2].b*conv*b, q->colors[2].a*conv*a);
		gfx_setcolorvertex(3, q->colors[3].r*conv*r, q->colors[3].g*conv*g, q->colors[3].b*conv*b, q->colors[3].a*conv*a);

		POINT *points = q->points;
	
		if(rot != 0)
		{
			static POINT rotated[4];
			rotated[0] = q->points[0];
			rotated[1] = q->points[1];
			rotated[2] = q->points[2];
			rotated[3] = q->points[3];
			points = rotated;
			
			rotate(&q->points[4], &rotated[0], rot);
			rotate(&q->points[4], &rotated[1], rot);
			rotate(&q->points[4], &rotated[2], rot);
			rotate(&q->points[4], &rotated[3], rot);
		}
		
		gfx_quads_draw_freeform(
			fx2f(points[0].x)+offset_x, fx2f(points[0].y)+offset_y,
			fx2f(points[1].x)+offset_x, fx2f(points[1].y)+offset_y,
			fx2f(points[2].x)+offset_x, fx2f(points[2].y)+offset_y,
			fx2f(points[3].x)+offset_x, fx2f(points[3].y)+offset_y
		);
	}
	gfx_quads_end();	
}


void render_tilemap(TILE *tiles, int w, int h, float scale)
{
			//gfx_texture_set(img_get(tmap->image));
	float screen_x0, screen_y0, screen_x1, screen_y1;
	gfx_getscreen(&screen_x0, &screen_y0, &screen_x1, &screen_y1);

	// calculate the final pixelsize for the tiles	
	float tile_pixelsize = 1024/32.0f;
	float final_tilesize = scale/(screen_x1-screen_x0) * gfx_screenwidth();
	float final_tilesize_scale = final_tilesize/tile_pixelsize;
	
	gfx_quads_begin();
	
	int starty = (int)(screen_y0/scale)-1;
	int startx = (int)(screen_x0/scale)-1;
	int endy = (int)(screen_y1/scale)+1;
	int endx = (int)(screen_x1/scale)+1;
	
	// adjust the texture shift according to mipmap level
	float texsize = 1024.0f;
	float frac = (1.25f/texsize) * (1/final_tilesize_scale);
	float nudge = (0.5f/texsize) * (1/final_tilesize_scale);

	for(int y = starty; y < endy; y++)
		for(int x = startx; x < endx; x++)
		{
			int mx = x;
			int my = y;
			if(mx<0)
				continue; // mx = 0;
			if(mx>=w)
				continue; // mx = w-1;
			if(my<0)
				continue; // my = 0;
			if(my>=h)
				continue; // my = h-1;
			
			int c = mx + my*w;
				
			unsigned char index = tiles[c].index;
			if(index)
			{
				unsigned char flags = tiles[c].flags;
				int tx = index%16;
				int ty = index/16;
				int px0 = tx*(1024/16);
				int py0 = ty*(1024/16);
				int px1 = (tx+1)*(1024/16)-1;
				int py1 = (ty+1)*(1024/16)-1;
				
				float u0 = nudge + px0/texsize+frac;
				float v0 = nudge + py0/texsize+frac;
				float u1 = nudge + px1/texsize-frac;
				float v1 = nudge + py1/texsize-frac;
				
				if(flags&TILEFLAG_VFLIP)
				{
					float tmp = u0;
					u0 = u1;
					u1 = tmp;
				}

				if(flags&TILEFLAG_HFLIP)
				{
					float tmp = v0;
					v0 = v1;
					v1 = tmp;
				}
				
				gfx_quads_setsubset(u0,v0,u1,v1);

				gfx_quads_drawTL(x*scale, y*scale, scale, scale);
			}
		}
	
	gfx_quads_end();
}
