/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_client_interface.h>
#include "../g_math.h"
#include "gc_client.h"

void render_eval_envelope(ENVPOINT *points, int num_points, int channels, float time, float *result)
{
	if(num_points == 0)
	{
		result[0] = 0;
		result[1] = 0;
		result[2] = 0;
		result[3] = 0;
		return;
	}
	
	if(num_points == 1)
	{
		result[0] = fx2f(points[0].values[0]);
		result[1] = fx2f(points[0].values[1]);
		result[2] = fx2f(points[0].values[2]);
		result[3] = fx2f(points[0].values[3]);
		return;
	}
	
	time = fmod(time, points[num_points-1].time/1000.0f)*1000.0f;
	for(int i = 0; i < num_points-1; i++)
	{
		if(time >= points[i].time && time <= points[i+1].time)
		{
			float delta = points[i+1].time-points[i].time;
			float a = (time-points[i].time)/delta;


			if(points[i].curvetype == CURVETYPE_SMOOTH)
				a = -2*a*a*a + 3*a*a; // second hermite basis
			else if(points[i].curvetype == CURVETYPE_SLOW)
				a = a*a*a;
			else if(points[i].curvetype == CURVETYPE_FAST)
			{
				a = 1-a;
				a = 1-a*a*a;
			}
			else if (points[i].curvetype == CURVETYPE_STEP)
				a = 0;
			else
			{
				// linear
			}
					
			for(int c = 0; c < channels; c++)
			{
				float v0 = fx2f(points[i].values[c]);
				float v1 = fx2f(points[i+1].values[c]);
				result[c] = v0 + (v1-v0) * a;
			}
			
			return;
		}
	}
	
	result[0] = fx2f(points[num_points-1].values[0]);
	result[1] = fx2f(points[num_points-1].values[1]);
	result[2] = fx2f(points[num_points-1].values[2]);
	result[3] = fx2f(points[num_points-1].values[3]);
	return;
}


static void rotate(POINT *center, POINT *point, float rotation)
{
	int x = point->x - center->x;
	int y = point->y - center->y;
	point->x = (int)(x * cosf(rotation) - y * sinf(rotation) + center->x);
	point->y = (int)(x * sinf(rotation) + y * cosf(rotation) + center->y);
}

void render_quads(QUAD *quads, int num_quads, void (*eval)(float time_offset, int env, float *channels))
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
		
		// TODO: fix this
		if(q->pos_env >= 0)
		{
			float channels[4];
			eval(q->pos_env_offset/1000.0f, q->pos_env, channels);
			offset_x = channels[0];
			offset_y = channels[1];
			rot = channels[2]/360.0f*pi*2;
		}
		
		if(q->color_env >= 0)
		{
			float channels[4];
			eval(q->color_env_offset/1000.0f, q->color_env, channels);
			r = channels[0];
			g = channels[1];
			b = channels[2];
			a = channels[3];
		}
		
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


void render_tilemap(TILE *tiles, int w, int h, float scale, vec4 color, int flags)
{
			//gfx_texture_set(img_get(tmap->image));
	float screen_x0, screen_y0, screen_x1, screen_y1;
	gfx_getscreen(&screen_x0, &screen_y0, &screen_x1, &screen_y1);

	// calculate the final pixelsize for the tiles	
	float tile_pixelsize = 1024/32.0f;
	float final_tilesize = scale/(screen_x1-screen_x0) * gfx_screenwidth();
	float final_tilesize_scale = final_tilesize/tile_pixelsize;
	
	gfx_quads_begin();
	gfx_setcolor(color.r, color.g, color.b, color.a);
	
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
			
			if(flags)
			{
				if(mx<0)
					mx = 0;
				if(mx>=w)
					mx = w-1;
				if(my<0)
					my = 0;
				if(my>=h)
					my = h-1;
			}
			else
			{
				if(mx<0)
					continue; // mx = 0;
				if(mx>=w)
					continue; // mx = w-1;
				if(my<0)
					continue; // my = 0;
				if(my>=h)
					continue; // my = h-1;
			}
			
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
