/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <math.h>
#include <engine/e_interface.h>
#include <engine/e_config.h>
#include "../generated/gc_data.h"
#include "../g_protocol.h"
#include "../g_math.h"
#include "gc_render.h"
#include "gc_anim.h"
#include "gc_client.h"

static float sprite_w_scale;
static float sprite_h_scale;

void select_sprite(sprite *spr, int flags, int sx, int sy)
{
	int x = spr->x+sx;
	int y = spr->y+sy;
	int w = spr->w;
	int h = spr->h;
	int cx = spr->set->gridx;
	int cy = spr->set->gridy;

	float f = sqrtf(h*h + w*w);
	sprite_w_scale = w/f;
	sprite_h_scale = h/f;
	
	float x1 = x/(float)cx;
	float x2 = (x+w)/(float)cx;
	float y1 = y/(float)cy;
	float y2 = (y+h)/(float)cy;
	float temp = 0;

	if(flags&SPRITE_FLAG_FLIP_Y)
	{
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	if(flags&SPRITE_FLAG_FLIP_X)
	{
		temp = x1;
		x1 = x2;
		x2 = temp;
	}
	
	gfx_quads_setsubset(x1, y1, x2, y2);
}

void select_sprite(int id, int flags, int sx, int sy)
{
	if(id < 0 || id > data->num_sprites)
		return;
	select_sprite(&data->sprites[id], flags, sx, sy);
}

void draw_sprite(float x, float y, float size)
{
	gfx_quads_draw(x, y, size*sprite_w_scale, size*sprite_h_scale);
}



void draw_round_rect_ext(float x, float y, float w, float h, float r, int corners)
{
	int num = 8;
	for(int i = 0; i < num; i+=2)
	{
		float a1 = i/(float)num * pi/2;
		float a2 = (i+1)/(float)num * pi/2;
		float a3 = (i+2)/(float)num * pi/2;
		float ca1 = cosf(a1);
		float ca2 = cosf(a2);
		float ca3 = cosf(a3);
		float sa1 = sinf(a1);
		float sa2 = sinf(a2);
		float sa3 = sinf(a3);

		if(corners&1) // TL
		gfx_quads_draw_freeform(
			x+r, y+r,
			x+(1-ca1)*r, y+(1-sa1)*r,
			x+(1-ca3)*r, y+(1-sa3)*r,
			x+(1-ca2)*r, y+(1-sa2)*r);

		if(corners&2) // TR
		gfx_quads_draw_freeform(
			x+w-r, y+r,
			x+w-r+ca1*r, y+(1-sa1)*r,
			x+w-r+ca3*r, y+(1-sa3)*r,
			x+w-r+ca2*r, y+(1-sa2)*r);

		if(corners&4) // BL
		gfx_quads_draw_freeform(
			x+r, y+h-r,
			x+(1-ca1)*r, y+h-r+sa1*r,
			x+(1-ca3)*r, y+h-r+sa3*r,
			x+(1-ca2)*r, y+h-r+sa2*r);

		if(corners&8) // BR
		gfx_quads_draw_freeform(
			x+w-r, y+h-r,
			x+w-r+ca1*r, y+h-r+sa1*r,
			x+w-r+ca3*r, y+h-r+sa3*r,
			x+w-r+ca2*r, y+h-r+sa2*r);
	}

	gfx_quads_drawTL(x+r, y+r, w-r*2, h-r*2); // center
	gfx_quads_drawTL(x+r, y, w-r*2, r); // top
	gfx_quads_drawTL(x+r, y+h-r, w-r*2, r); // bottom
	gfx_quads_drawTL(x, y+r, r, h-r*2); // left
	gfx_quads_drawTL(x+w-r, y+r, r, h-r*2); // right
	
	if(!(corners&1)) gfx_quads_drawTL(x, y, r, r); // TL
	if(!(corners&2)) gfx_quads_drawTL(x+w, y, -r, r); // TR
	if(!(corners&4)) gfx_quads_drawTL(x, y+h, r, -r); // BL
	if(!(corners&8)) gfx_quads_drawTL(x+w, y+h, -r, -r); // BR
}

void draw_round_rect(float x, float y, float w, float h, float r)
{
	draw_round_rect_ext(x,y,w,h,r,0xf);
}

void render_tee(animstate *anim, tee_render_info *info, int emote, vec2 dir, vec2 pos)
{
	vec2 direction = dir;
	vec2 position = pos;

	//gfx_texture_set(data->images[IMAGE_CHAR_DEFAULT].id);
	gfx_texture_set(info->texture);
	gfx_quads_begin();
	//gfx_quads_draw(pos.x, pos.y-128, 128, 128);

	// first pass we draw the outline
	// second pass we draw the filling
	for(int p = 0; p < 2; p++)
	{
		int outline = p==0 ? 1 : 0;

		for(int f = 0; f < 2; f++)
		{
			float animscale = info->size * 1.0f/64.0f;
			float basesize = info->size;
			if(f == 1)
			{
				gfx_quads_setrotation(anim->body.angle*pi*2);

				// draw body
				gfx_setcolor(info->color_body.r, info->color_body.g, info->color_body.b, info->color_body.a);
				vec2 body_pos = position + vec2(anim->body.x, anim->body.y)*animscale;
				select_sprite(outline?SPRITE_TEE_BODY_OUTLINE:SPRITE_TEE_BODY, 0, 0, 0);
				gfx_quads_draw(body_pos.x, body_pos.y, basesize, basesize);

				// draw eyes
				if(p == 1)
				{
					switch (emote)
					{
						case EMOTE_PAIN:
							select_sprite(SPRITE_TEE_EYE_PAIN, 0, 0, 0);
							break;
						case EMOTE_HAPPY:
							select_sprite(SPRITE_TEE_EYE_HAPPY, 0, 0, 0);
							break;
						case EMOTE_SURPRISE:
							select_sprite(SPRITE_TEE_EYE_SURPRISE, 0, 0, 0);
							break;
						case EMOTE_ANGRY:
							select_sprite(SPRITE_TEE_EYE_ANGRY, 0, 0, 0);
							break;
						default:
							select_sprite(SPRITE_TEE_EYE_NORMAL, 0, 0, 0);
							break;
					}
					
					float eyescale = basesize*0.40f;
					float h = emote == EMOTE_BLINK ? basesize*0.15f : eyescale;
					float eyeseparation = (0.075f - 0.010f*fabs(direction.x))*basesize;
					vec2 offset = vec2(direction.x*0.125f, -0.05f+direction.y*0.10f)*basesize;
					gfx_quads_draw(body_pos.x-eyeseparation+offset.x, body_pos.y+offset.y, eyescale, h);
					gfx_quads_draw(body_pos.x+eyeseparation+offset.x, body_pos.y+offset.y, -eyescale, h);
				}
			}

			// draw feet
			gfx_setcolor(info->color_feet.r, info->color_feet.g, info->color_feet.b, info->color_feet.a);
			select_sprite(outline?SPRITE_TEE_FOOT_OUTLINE:SPRITE_TEE_FOOT, 0, 0, 0);

			keyframe *foot = f ? &anim->front_foot : &anim->back_foot;

			float w = basesize;
			float h = basesize/2;

			gfx_quads_setrotation(foot->angle*pi*2);
			gfx_quads_draw(position.x+foot->x*animscale, position.y+foot->y*animscale, w, h);
		}
	}

	gfx_quads_end();
}

static void calc_screen_params(float amount, float wmax, float hmax, float aspect, float *w, float *h)
{
	float f = sqrt(amount) / sqrt(aspect);
	*w = f*aspect;
	*h = f;
	
	// limit the view
	if(*w > wmax)
	{
		*w = wmax;
		*h = *w/aspect;
	}
	
	if(*h > hmax)
	{
		*h = hmax;
		*w = *h*aspect;
	}
}

void mapscreen_to_world(float center_x, float center_y, float parallax_x, float parallax_y,
	float offset_x, float offset_y, float aspect, float zoom, float *points)
{
	float width, height;
	calc_screen_params(1300*1000, 1500, 1050, aspect, &width, &height);
	center_x *= parallax_x;
	center_y *= parallax_y;
	width *= zoom;
	height *= zoom;
	points[0] = offset_x+center_x-width/2;
	points[1] = offset_y+center_y-height/2;
	points[2] = offset_x+center_x+width/2;
	points[3] = offset_y+center_y+height/2;
}
