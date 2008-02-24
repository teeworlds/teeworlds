/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <math.h>
#include <engine/e_client_interface.h>
#include <engine/e_config.h>
#include <game/generated/gc_data.h>
#include <game/g_protocol.h>
#include <game/g_math.h>
#include <game/g_layers.h>
#include "gc_render.h"
#include "gc_anim.h"
#include "gc_client.h"
#include "gc_map_image.h"

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

void ui_draw_rect(const RECT *r, vec4 color, int corners, float rounding)
{
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(color.r, color.g, color.b, color.a);
	draw_round_rect_ext(r->x,r->y,r->w,r->h,rounding*ui_scale(), corners);
	gfx_quads_end();
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
			select_sprite((outline||!info->got_airjump)?SPRITE_TEE_FOOT_OUTLINE:SPRITE_TEE_FOOT, 0, 0, 0);

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

static void mapscreen_to_group(float center_x, float center_y, MAPITEM_GROUP *group)
{
	float points[4];
	mapscreen_to_world(center_x, center_y, group->parallax_x/100.0f, group->parallax_y/100.0f,
		group->offset_x, group->offset_y, gfx_screenaspect(), 1.0f, points);
	gfx_mapscreen(points[0], points[1], points[2], points[3]);
}

static void envelope_eval(float time_offset, int env, float *channels)
{
	channels[0] = 0;
	channels[1] = 0;
	channels[2] = 0;
	channels[3] = 0;

	ENVPOINT *points;

	{
		int start, num;
		map_get_type(MAPITEMTYPE_ENVPOINTS, &start, &num);
		if(num)
			points = (ENVPOINT *)map_get_item(start, 0, 0);
	}
	
	int start, num;
	map_get_type(MAPITEMTYPE_ENVELOPE, &start, &num);
	
	if(env >= num)
		return;
	
	MAPITEM_ENVELOPE *item = (MAPITEM_ENVELOPE *)map_get_item(start+env, 0, 0);
	render_eval_envelope(points+item->start_point, item->num_points, 4, client_localtime()+time_offset, channels);
}

void render_layers(float center_x, float center_y, int pass)
{
	bool passed_gamelayer = false;
	for(int g = 0; g < layers_num_groups(); g++)
	{
		MAPITEM_GROUP *group = layers_get_group(g);
		
		mapscreen_to_group(center_x, center_y, group);
		
		for(int l = 0; l < group->num_layers; l++)
		{
			MAPITEM_LAYER *layer = layers_get_layer(group->start_layer+l);
			bool render = false;
			bool is_game_layer = false;
			
			if(layer == (MAPITEM_LAYER*)layers_game_layer())
			{
				is_game_layer = true;
				passed_gamelayer = 1;
			}
				
			if(pass == 0)
			{
				if(passed_gamelayer)
					return;
				render = true;
			}
			else
			{
				if(passed_gamelayer && !is_game_layer)
					render = true;
			}
			
			if(render)
			{
				if(layer->type == LAYERTYPE_TILES)
				{
					MAPITEM_LAYER_TILEMAP *tmap = (MAPITEM_LAYER_TILEMAP *)layer;
					if(tmap->image == -1)
						gfx_texture_set(-1);
					else
						gfx_texture_set(img_get(tmap->image));
					TILE *tiles = (TILE *)map_get_data(tmap->data);
					render_tilemap(tiles, tmap->width, tmap->height, 32.0f, vec4(1,1,1,1), 1);
				}
				else if(layer->type == LAYERTYPE_QUADS)
				{
					MAPITEM_LAYER_QUADS *qlayer = (MAPITEM_LAYER_QUADS *)layer;
					if(qlayer->image == -1)
						gfx_texture_set(-1);
					else
						gfx_texture_set(img_get(qlayer->image));
					QUAD *quads = (QUAD *)map_get_data_swapped(qlayer->data);
					render_quads(quads, qlayer->num_quads, envelope_eval);
				}
			}
		}
	}
}

static void render_items()
{
	int num = snap_num_items(SNAP_CURRENT);
	for(int i = 0; i < num; i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PROJECTILE)
		{
			render_projectile((const NETOBJ_PROJECTILE *)data, item.id);
		}
		else if(item.type == NETOBJTYPE_POWERUP)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if(prev)
				render_powerup((const NETOBJ_POWERUP *)prev, (const NETOBJ_POWERUP *)data);
		}
		else if(item.type == NETOBJTYPE_LASER)
		{
			render_laser((const NETOBJ_LASER *)data);
		}
		else if(item.type == NETOBJTYPE_FLAG)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			if (prev)
				render_flag((const NETOBJ_FLAG *)prev, (const NETOBJ_FLAG *)data);
		}
	}

	// render extra projectiles	
	for(int i = 0; i < extraproj_num; i++)
	{
		if(extraproj_projectiles[i].start_tick < client_tick())
		{
			extraproj_projectiles[i] = extraproj_projectiles[extraproj_num-1];
			extraproj_num--;
		}
		else
			render_projectile(&extraproj_projectiles[i], 0);
	}
}


static void render_players()
{
	int num = snap_num_items(SNAP_CURRENT);
	for(int i = 0; i < num; i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PLAYER_CHARACTER)
		{
			const void *prev = snap_find_item(SNAP_PREV, item.type, item.id);
			const void *prev_info = snap_find_item(SNAP_PREV, NETOBJTYPE_PLAYER_INFO, item.id);
			const void *info = snap_find_item(SNAP_CURRENT, NETOBJTYPE_PLAYER_INFO, item.id);

			if(prev && prev_info && info)
			{
				render_player(
						(const NETOBJ_PLAYER_CHARACTER *)prev,
						(const NETOBJ_PLAYER_CHARACTER *)data,
						(const NETOBJ_PLAYER_INFO *)prev_info,
						(const NETOBJ_PLAYER_INFO *)info
					);
			}
		}
	}
}

// renders the complete game world
void render_world(float center_x, float center_y, float zoom)
{
	// render background layers
	render_layers(center_x, center_y, 0);

	// render trails
	particle_render(PARTGROUP_PROJECTILE_TRAIL);

	// render items
	render_items();

	// render players above all
	render_players();

	// render particles
	particle_render(PARTGROUP_EXPLOSIONS);
	particle_render(PARTGROUP_GENERAL);
	
	if(config.dbg_flow)
		flow_dbg_render();

	// render foreground layers
	render_layers(center_x, center_y, 1);

	// render damage indications
	render_damage_indicators();
}
