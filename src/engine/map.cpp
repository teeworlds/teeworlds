#include <baselib/system.h>
#include <baselib/stream/file.h>

#include "datafile.h"

static datafile *map;

void *map_get_data(int index)
{
	return datafile_get_data(map, index);
}

void *map_get_item(int index, int *type, int *id)
{
	return datafile_get_item(map, index, type, id);
}

void map_get_type(int type, int *start, int *num)
{
	datafile_get_type(map, type, start, num);
}

void *map_find_item(int type, int id)
{
	return datafile_find_item(map, type, id);
}

int map_num_items()
{
	return datafile_num_items(map);
}

void map_unload()
{
	datafile_unload(map);
	map = 0x0;
}

int map_is_loaded()
{
	return map != 0;
}

int map_load(const char *mapname)
{
	map = datafile_load(mapname);
	return map != 0;
}
