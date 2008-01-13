#include "g_mapitems.h"

void layers_init();

MAPITEM_LAYER_TILEMAP *layers_game();

int layers_num_groups();
MAPITEM_GROUP *layers_get_group(int index);
MAPITEM_LAYER *layers_get_layer(int index);


