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
	map_get_type(MAPITEMTYPE_LAYER, &layers_start, &layers_num);
	
	for(int g = 0; g < layers_num_groups(); g++)
	{
		MAPITEM_GROUP *group = layers_get_group(g);
		for(int l = 0; l < group->num_layers; l++)
		{
			MAPITEM_LAYER *layer = layers_get_layer(group->start_layer+l);
			
			if(layer->type == LAYERTYPE_TILES)
			{
				MAPITEM_LAYER_TILEMAP *tilemap = (MAPITEM_LAYER_TILEMAP *)layer;
				if(tilemap->flags&1)
				{
					game_layer = tilemap;
					game_group = group;
				}
			}			
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

