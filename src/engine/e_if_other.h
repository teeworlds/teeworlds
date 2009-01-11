/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_IF_OTHER_H
#define ENGINE_IF_OTHER_H

/*
	Title: Engine Interface
*/

#include <base/system.h>
#include "e_keys.h"

enum 
{
	SERVER_TICK_SPEED=50,
	MAX_CLIENTS=16,
	
	SNAP_CURRENT=0,
	SNAP_PREV=1,

	MASK_NONE=0,
	MASK_SET,
	MASK_ZERO,

	SNDFLAG_LOOP=1,
	SNDFLAG_POS=2,
	SNDFLAG_ALL=3,
	
	MAX_NAME_LENGTH=32
};

enum
{
	SRVFLAG_PASSWORD = 0x1,
};

/*
	Structure: SNAP_ITEM
*/
typedef struct
{
	int type;
	int id;
	int datasize;
} SNAP_ITEM;

/*
	Structure: CLIENT_INFO
*/
typedef struct
{
	const char *name;
	int latency;
} CLIENT_INFO;

typedef struct PERFORMACE_INFO_t
{
	const char *name;
	struct PERFORMACE_INFO_t *parent;
	struct PERFORMACE_INFO_t *first_child;
	struct PERFORMACE_INFO_t *next_child;
	int tick;
	int64 start;
	int64 total;
	int64 biggest;
	int64 last_delta;
} PERFORMACE_INFO;

void perf_init();
void perf_next();
void perf_start(PERFORMACE_INFO *info);
void perf_end();
void perf_dump();

int gfx_init();
void gfx_shutdown();
void gfx_swap();
int gfx_window_active();
int gfx_window_open();

void gfx_set_vsync(int val);

int snd_init();
int snd_shutdown();
int snd_update();

int map_load(const char *mapname);
void map_unload();

void map_set(void *m);

/*
#include "e_if_client.h"
#include "e_if_server.h"
#include "e_if_snd.h"
#include "e_if_gfx.h"
#include "e_if_inp.h"
#include "e_if_msg.h"
#include "e_if_mod.h"*/

/*
	Section: Map
*/

/*
	Function: map_is_loaded
		Checks if a map is loaded.
		
	Returns:
		Returns 1 if the button is pressed, otherwise 0.
*/
int map_is_loaded();

/*
	Function: map_num_items
		Checks the number of items in the loaded map.
		
	Returns:
		Returns the number of items. 0 if no map is loaded.
*/
int map_num_items();

/*
	Function: map_find_item
		Searches the map for an item.
	
	Arguments:
		type - Item type.
		id - Item ID.
	
	Returns:
		Returns a pointer to the item if it exists, otherwise it returns NULL.
*/
void *map_find_item(int type, int id);

/*
	Function: map_get_item
		Gets an item from the loaded map from index.
	
	Arguments:
		index - Item index.
		type - Pointer that recives the item type (can be NULL).
		id - Pointer that recives the item id (can be NULL).
	
	Returns:
		Returns a pointer to the item if it exists, otherwise it returns NULL.
*/
void *map_get_item(int index, int *type, int *id);

/*
	Function: map_get_type
		Gets the index range of an item type.
	
	Arguments:
		type - Item type to search for.
		start - Pointer that recives the starting index.
		num - Pointer that recives the number of items.
	
	Returns:
		If the item type is not in the map, start and num will be set to 0.
*/
void map_get_type(int type, int *start, int *num);

/*
	Function: map_get_data
		Fetches a pointer to a raw data chunk in the map.
	
	Arguments:
		index - Index to the data to fetch.
	
	Returns:
		A pointer to the raw data, otherwise 0.
*/
void *map_get_data(int index);

/*
	Function: map_get_data_swapped
		TODO
		
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void *map_get_data_swapped(int index);

/*
	Section: Network (Server)
*/
/*
	Function: snap_new_item
		Creates a new item that should be sent.
	
	Arguments:
		type - Type of the item.
		id - ID of the item.
		size - Size of the item.
	
	Returns:
		A pointer to the item data, otherwise 0.
	
	Remarks:
		The item data should only consist pf 4 byte integers as
		they are subject to byte swapping. This means that the size
		argument should be dividable by 4.
*/
void *snap_new_item(int type, int id, int size);

/*
	Section:Section: Network (Client)
*/
/*
	Function: snap_num_items
		Check the number of items in a snapshot.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
	
	Returns:
		The number of items in the snapshot.
*/
int snap_num_items(int snapid);

/*
	Function: snap_get_item
		Gets an item from a snapshot.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
		index - Index of the item.
		item - Pointer that recives the item info.
	
	Returns:
		Returns a pointer to the item if it exists, otherwise NULL.
*/
void *snap_get_item(int snapid, int index, SNAP_ITEM *item);

/*
	Function: snap_find_item
		Searches a snapshot for an item.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
		type - Type of the item.
		id - ID of the item.
	
	Returns:
		Returns a pointer to the item if it exists, otherwise NULL.
*/
void *snap_find_item(int snapid, int type, int id);

/*
	Function: snap_invalidate_item
		Marks an item as invalid byt setting type and id to 0xffffffff.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
		index - Index of the item.
*/
void snap_invalidate_item(int snapid, int index);

/*
	Function: snap_input
		Sets the input data to send to the server.
	
	Arguments:
		data - Pointer to the data.
		size - Size of the data.

	Remarks:
		The data should only consist of 4 bytes integer as they are
		subject to byte swapping.
*/
void snap_input(void *data, int size);

/*
	Function: snap_set_staticsize
		Tells the engine how big a specific item always will be. This
		helps the engine to compress snapshots better.
	
	Arguments:
		type - Item type
		size - Size of the data.
		
	Remarks:
		Size must be in a multiple of 4.
*/
void snap_set_staticsize(int type, int size);

/* message packing */
enum
{
	MSGFLAG_VITAL=1,
	MSGFLAG_FLUSH=2,
	MSGFLAG_NORECORD=4,
	MSGFLAG_RECORD=8,
	MSGFLAG_NOSEND=16
};

/* message sending */
/*
	Function: server_send_msg
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_send_msg(int client_id); /* client_id == -1 == broadcast */

/*
	Function: client_send_msg
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_send_msg();
/* undocumented graphics stuff */

/* server snap id */
/*
	Function: snap_new_id
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int snap_new_id();

/*
	Function: snap_free_id
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void snap_free_id(int id);

/* other */
/*
	Function: map_unload_data
		TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void map_unload_data(int index);

#endif
