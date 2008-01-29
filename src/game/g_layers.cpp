#include <engine/e_common_interface.h>
#include "g_layers.h"

static MAPITEM_LAYER_TILEMAP *game_layer = 0;
static MAPITEM_GROUP *game_group = 0;

static int groups_start = 0;
static int groups_num = 0;
static int layers_start = 0;
static int layers_num = 0;

void layers_init()
{
	map_get_type(MAPITEMTYPE_GROUP, &groups_start, &groups_num);
	
	{
		int p = 0;
		map_get_type(MAPITEMTYPE_LAYER, &layers_start, &layers_num);
			
		for(int i = 0; i < layers_num; i++)
		{
			MAPITEM_LAYER *layer = (MAPITEM_LAYER *)map_get_item(layers_start+i, 0, 0);
			if(layer->type == LAYERTYPE_TILES)
			{
				MAPITEM_LAYER_TILEMAP *tilemap = (MAPITEM_LAYER_TILEMAP *)layer;
				
				if(p)
				{
					p--;
					if(p == 0)
						tilemap->flags |= 2;
				}
				
				if(tilemap->flags&1)
				{
					dbg_msg("layers", "game");
					game_layer = tilemap;
					p = 2;
				}
			}
			dbg_msg("layers", "%d %d", i, layer->type);
		}
	}
}

int layers_num_groups() { return groups_num; }
MAPITEM_GROUP *layers_get_group(int index)
{
	return (MAPITEM_GROUP *)map_get_item(groups_start+index, 0, 0);
}

MAPITEM_LAYER *layers_get_layer(int index)
{
	return (MAPITEM_LAYER *)map_get_item(layers_start+index, 0, 0);
}

MAPITEM_LAYER_TILEMAP *layers_game_layer()
{
	return game_layer;
}

MAPITEM_GROUP *layers_game_group()
{
	return game_group;
}

