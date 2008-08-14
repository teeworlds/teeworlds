/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_SNAPSHOT_H
#define ENGINE_SNAPSHOT_H

#include <base/system.h>

/* SNAPSHOT */

enum
{
	MAX_SNAPSHOT_SIZE=64*1024
};

typedef struct
{
	int type_and_id;
} SNAPSHOT_ITEM;

typedef struct 
{
	int data_size;
	int num_items;
} SNAPSHOT;

int *snapitem_data(SNAPSHOT_ITEM *item);
int snapitem_type(SNAPSHOT_ITEM *item);
int snapitem_id(SNAPSHOT_ITEM *item);
int snapitem_key(SNAPSHOT_ITEM *item);

int *snapshot_offsets(SNAPSHOT *snap);
char *snapshot_datastart(SNAPSHOT *snap);

SNAPSHOT_ITEM *snapshot_get_item(SNAPSHOT *snap, int index);
int snapshot_get_item_datasize(SNAPSHOT *snap, int index);
int snapshot_get_item_index(SNAPSHOT *snap, int key);

void *snapshot_empty_delta();
int snapshot_crc(SNAPSHOT *snap);
void snapshot_debug_dump(SNAPSHOT *snap);
int snapshot_create_delta(SNAPSHOT *from, SNAPSHOT *to, void *data);
int snapshot_unpack_delta(SNAPSHOT *from, SNAPSHOT *to, void *data, int data_size);

/* SNAPSTORAGE */

typedef struct SNAPSTORAGE_HOLDER_t
{
	struct SNAPSTORAGE_HOLDER_t *prev;
	struct SNAPSTORAGE_HOLDER_t *next;
	
	int64 tagtime;
	int tick;
	
	int snap_size;
	SNAPSHOT *snap;
	SNAPSHOT *alt_snap;
} SNAPSTORAGE_HOLDER;
 
typedef struct SNAPSTORAGE_t
{
	SNAPSTORAGE_HOLDER *first;
	SNAPSTORAGE_HOLDER *last;
} SNAPSTORAGE;

void snapstorage_init(SNAPSTORAGE *ss);
void snapstorage_purge_all(SNAPSTORAGE *ss);
void snapstorage_purge_until(SNAPSTORAGE *ss, int tick);
void snapstorage_add(SNAPSTORAGE *ss, int tick, int64 tagtime, int data_size, void *data, int create_alt);
int snapstorage_get(SNAPSTORAGE *ss, int tick, int64 *tagtime, SNAPSHOT **data, SNAPSHOT **alt_data);

/* SNAPBUILD */

enum
{
	SNAPBUILD_MAX_ITEMS = 1024*2
};

typedef struct SNAPBUILD
{
	char data[MAX_SNAPSHOT_SIZE];
	int data_size;

	int offsets[SNAPBUILD_MAX_ITEMS];
	int num_items;
} SNAPBUILD;

void snapbuild_init(SNAPBUILD *sb);
SNAPSHOT_ITEM *snapbuild_get_item(SNAPBUILD *sb, int index);
int *snapbuild_get_item_data(SNAPBUILD *sb, int key);
int snapbuild_finish(SNAPBUILD *sb, void *snapdata);
void *snapbuild_new_item(SNAPBUILD *sb, int type, int id, int size);

#endif /* ENGINE_SNAPSHOT_H */
