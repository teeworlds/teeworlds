#include <game/layers.hpp>
#include <game/client/gameclient.hpp>
#include <game/client/component.hpp>
#include <game/client/render.hpp>

#include <game/client/components/camera.hpp>
#include <game/client/components/mapimages.hpp>

#include "maplayers.hpp"

MAPLAYERS::MAPLAYERS(int t)
{
	type = t;
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

void MAPLAYERS::on_render()
{
	if(client_state() != CLIENTSTATE_ONLINE && client_state() != CLIENTSTATE_DEMOPLAYBACK)
		return;
	
	RECT screen;
	gfx_getscreen(&screen.x, &screen.y, &screen.w, &screen.h);
	
	vec2 center = gameclient.camera->center;
	//float center_x = gameclient.camera->center.x;
	//float center_y = gameclient.camera->center.y;
	
	bool passed_gamelayer = false;
	
	for(int g = 0; g < layers_num_groups(); g++)
	{
		MAPITEM_GROUP *group = layers_get_group(g);
		
		if(!config.gfx_noclip && group->version >= 2 && group->use_clipping)
		{
			// set clipping
			float points[4];
			mapscreen_to_group(center.x, center.y, layers_game_group());
			gfx_getscreen(&points[0], &points[1], &points[2], &points[3]);
			float x0 = (group->clip_x - points[0]) / (points[2]-points[0]);
			float y0 = (group->clip_y - points[1]) / (points[3]-points[1]);
			float x1 = ((group->clip_x+group->clip_w) - points[0]) / (points[2]-points[0]);
			float y1 = ((group->clip_y+group->clip_h) - points[1]) / (points[3]-points[1]);
			
			gfx_clip_enable((int)(x0*gfx_screenwidth()), (int)(y0*gfx_screenheight()),
				(int)((x1-x0)*gfx_screenwidth()), (int)((y1-y0)*gfx_screenheight()));
		}		
		
		mapscreen_to_group(center.x, center.y, group);
		
		for(int l = 0; l < group->num_layers; l++)
		{
			MAPITEM_LAYER *layer = layers_get_layer(group->start_layer+l);
			bool render = false;
			bool is_game_layer = false;
			
			// skip rendering if detail layers if not wanted
			if(layer->flags&LAYERFLAG_DETAIL && !config.gfx_high_detail)
				continue;
			
			if(layer == (MAPITEM_LAYER*)layers_game_layer())
			{
				is_game_layer = true;
				passed_gamelayer = 1;
			}
				
			if(type == -1)
				render = true;
			else if(type == 0)
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
			
			if(render && !is_game_layer)
			{
				//layershot_begin();
				
				if(layer->type == LAYERTYPE_TILES)
				{
					MAPITEM_LAYER_TILEMAP *tmap = (MAPITEM_LAYER_TILEMAP *)layer;
					if(tmap->image == -1)
						gfx_texture_set(-1);
					else
						gfx_texture_set(gameclient.mapimages->get(tmap->image));
						
					TILE *tiles = (TILE *)map_get_data(tmap->data);
					gfx_blend_none();
					render_tilemap(tiles, tmap->width, tmap->height, 32.0f, vec4(1,1,1,1), TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_OPAQUE);
					gfx_blend_normal();
					render_tilemap(tiles, tmap->width, tmap->height, 32.0f, vec4(1,1,1,1), TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_TRANSPARENT);
				}
				else if(layer->type == LAYERTYPE_QUADS)
				{
					MAPITEM_LAYER_QUADS *qlayer = (MAPITEM_LAYER_QUADS *)layer;
					if(qlayer->image == -1)
						gfx_texture_set(-1);
					else
						gfx_texture_set(gameclient.mapimages->get(qlayer->image));

					QUAD *quads = (QUAD *)map_get_data_swapped(qlayer->data);
					
					gfx_blend_none();
					render_quads(quads, qlayer->num_quads, envelope_eval, LAYERRENDERFLAG_OPAQUE);
					gfx_blend_normal();
					render_quads(quads, qlayer->num_quads, envelope_eval, LAYERRENDERFLAG_TRANSPARENT);
				}
				
				//layershot_end();	
			}
		}
		if(!config.gfx_noclip)
			gfx_clip_disable();
	}
	
	if(!config.gfx_noclip)
		gfx_clip_disable();
	
	// reset the screen like it was before
	gfx_mapscreen(screen.x, screen.y, screen.w, screen.h);
}

