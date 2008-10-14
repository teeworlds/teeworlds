/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h>
#include "e_snapshot.h"
#include "e_engine.h"
#include "e_compression.h"
#include "e_common_interface.h"


/* TODO: strange arbitrary number */
static short item_sizes[64] = {0};

void snap_set_staticsize(int itemtype, int size)
{
	item_sizes[itemtype] = size;
}

int *snapitem_data(SNAPSHOT_ITEM *item) { return (int *)(item+1); }
int snapitem_type(SNAPSHOT_ITEM *item) { return item->type_and_id>>16; }
int snapitem_id(SNAPSHOT_ITEM *item) { return item->type_and_id&0xffff; }
int snapitem_key(SNAPSHOT_ITEM *item) { return item->type_and_id; }

int *snapshot_offsets(SNAPSHOT *snap) { return (int *)(snap+1); }
char *snapshot_datastart(SNAPSHOT *snap) { return (char*)(snapshot_offsets(snap)+snap->num_items); }

SNAPSHOT_ITEM *snapshot_get_item(SNAPSHOT *snap, int index)
{ return (SNAPSHOT_ITEM *)(snapshot_datastart(snap) + snapshot_offsets(snap)[index]); }

int snapshot_get_item_datasize(SNAPSHOT *snap, int index)
{
    if(index == snap->num_items-1)
        return (snap->data_size - snapshot_offsets(snap)[index]) - sizeof(SNAPSHOT_ITEM);
    return (snapshot_offsets(snap)[index+1] - snapshot_offsets(snap)[index]) - sizeof(SNAPSHOT_ITEM);
}

int snapshot_get_item_index(SNAPSHOT *snap, int key)
{
    /* TODO: OPT: this should not be a linear search. very bad */
    int i;
    for(i = 0; i < snap->num_items; i++)
    {
        if(snapitem_key(snapshot_get_item(snap, i)) == key)
            return i;
    }
    return -1;
}
typedef struct 
{
	int num;
	int keys[64];
	int index[64];
} ITEMLIST;
static ITEMLIST sorted[256];

static int snapshot_generate_hash(SNAPSHOT *snap)
{
	int i, key, hashid;

    for(i = 0; i < 256; i++)
    	sorted[i].num = 0;
    	
    for(i = 0; i < snap->num_items; i++)
    {
    	key = snapitem_key(snapshot_get_item(snap, i));
    	hashid = ((key>>8)&0xf0) | (key&0xf);
    	if(sorted[hashid].num != 64)
    	{
			sorted[hashid].index[sorted[hashid].num] = i;
			sorted[hashid].keys[sorted[hashid].num] = key;
			sorted[hashid].num++;
		}
	}
    return 0;
}

int snapshot_get_item_index_hashed(SNAPSHOT *snap, int key)
{
   	int hashid = ((key>>8)&0xf0) | (key&0xf);
   	int i;
   	for(i = 0; i < sorted[hashid].num; i++)
   	{
   		if(sorted[hashid].keys[i] == key)
   			return sorted[hashid].index[i];
	}
	
	return -1;
}

typedef struct
{
	int num_deleted_items;
	int num_update_items;
	int num_temp_items; /* needed? */
	int data[1];
	
	/*
	char *data_start() { return (char *)&offsets[num_deleted_items+num_update_items+num_temp_items]; }
	
	int deleted_item(int index) { return offsets[index]; }
	item *update_item(int index) { return (item *)(data_start() + offsets[num_deleted_items+index]); }
	item *temp_item(int index) { return (item *)(data_start() + offsets[num_deleted_items+num_update_items+index]); }
	*/
} SNAPSHOT_DELTA;


static const int MAX_ITEMS = 512;
static SNAPSHOT_DELTA empty = {0,0,0,{0}};

void *snapshot_empty_delta()
{
	return &empty;
}

int snapshot_crc(SNAPSHOT *snap)
{
	int crc = 0;
	int i, b;
	SNAPSHOT_ITEM *item;
	int size;
	
	for(i = 0; i < snap->num_items; i++)
	{
		item = snapshot_get_item(snap, i);
		size = snapshot_get_item_datasize(snap, i);
		
		for(b = 0; b < size/4; b++)
			crc += snapitem_data(item)[b];
	}
	return crc;
}

void snapshot_debug_dump(SNAPSHOT *snap)
{
	int size, i, b;
	SNAPSHOT_ITEM *item;
	
	dbg_msg("snapshot", "data_size=%d num_items=%d", snap->data_size, snap->num_items);
	for(i = 0; i < snap->num_items; i++)
	{
		item = snapshot_get_item(snap, i);
		size = snapshot_get_item_datasize(snap, i);
		dbg_msg("snapshot", "\ttype=%d id=%d", snapitem_type(item), snapitem_id(item));
		for(b = 0; b < size/4; b++)
			dbg_msg("snapshot", "\t\t%3d %12d\t%08x", b, snapitem_data(item)[b], snapitem_data(item)[b]);
	}
}

static int diff_item(int *past, int *current, int *out, int size)
{
	int needed = 0;
	while(size)
	{
		*out = *current-*past;
		needed |= *out;
		out++;
		past++;
		current++;
		size--;
	}
	
	return needed;
}

int snapshot_data_rate[0xffff] = {0};
int snapshot_data_updates[0xffff] = {0};
static int snapshot_current = 0;

static void undiff_item(int *past, int *diff, int *out, int size)
{
	while(size)
	{
		*out = *past+*diff;
		
		if(*diff == 0)
			snapshot_data_rate[snapshot_current] += 1;
		else
		{
			unsigned char buf[16];
			unsigned char *end = vint_pack(buf,  *diff);
			snapshot_data_rate[snapshot_current] += (int)(end - (unsigned char*)buf) * 8;
		}
		
		out++;
		past++;
		diff++;
		size--;
	}
}


/* TODO: OPT: this should be made much faster */
int snapshot_create_delta(SNAPSHOT *from, SNAPSHOT *to, void *dstdata)
{
	static PERFORMACE_INFO hash_scope = {"hash", 0};
	SNAPSHOT_DELTA *delta = (SNAPSHOT_DELTA *)dstdata;
	int *data = (int *)delta->data;
	int i, itemsize, pastindex;
	SNAPSHOT_ITEM *fromitem;
	SNAPSHOT_ITEM *curitem;
	SNAPSHOT_ITEM *pastitem;
	int count = 0;
	int size_count = 0;
	
	delta->num_deleted_items = 0;
	delta->num_update_items = 0;
	delta->num_temp_items = 0;
	
	perf_start(&hash_scope);
	snapshot_generate_hash(to);
	perf_end();

	/* pack deleted stuff */
	{
		static PERFORMACE_INFO scope = {"delete", 0};
		perf_start(&scope);
		
		for(i = 0; i < from->num_items; i++)
		{
			fromitem = snapshot_get_item(from, i);
			if(snapshot_get_item_index_hashed(to, (snapitem_key(fromitem))) == -1)
			{
				/* deleted */
				delta->num_deleted_items++;
				*data = snapitem_key(fromitem);
				data++;
			}
		}
		
		perf_end();
	}
	
	perf_start(&hash_scope);
	snapshot_generate_hash(from);
	perf_end();
	
	/* pack updated stuff */
	{
		static PERFORMACE_INFO scope = {"update", 0};
		int pastindecies[1024];
		perf_start(&scope);

		/* fetch previous indices */
		/* we do this as a separate pass because it helps the cache */
		{
			static PERFORMACE_INFO scope = {"find", 0};
			perf_start(&scope);
			for(i = 0; i < to->num_items; i++)
			{
				curitem = snapshot_get_item(to, i);  /* O(1) .. O(n) */
				pastindecies[i] = snapshot_get_item_index_hashed(from, snapitem_key(curitem)); /* O(n) .. O(n^n)*/
			}
			perf_end();
		}		
			
		for(i = 0; i < to->num_items; i++)
		{
			/* do delta */
			itemsize = snapshot_get_item_datasize(to, i); /* O(1) .. O(n) */
			curitem = snapshot_get_item(to, i);  /* O(1) .. O(n) */
			pastindex = pastindecies[i];
			
			if(pastindex != -1)
			{
				static PERFORMACE_INFO scope = {"diff", 0};
				int *item_data_dst = data+3;
				perf_start(&scope);
		
				pastitem = snapshot_get_item(from, pastindex);
				
				if(item_sizes[snapitem_type(curitem)])
					item_data_dst = data+2;
				
				if(diff_item((int*)snapitem_data(pastitem), (int*)snapitem_data(curitem), item_data_dst, itemsize/4))
				{
					
					*data++ = snapitem_type(curitem);
					*data++ = snapitem_id(curitem);
					if(!item_sizes[snapitem_type(curitem)])
						*data++ = itemsize/4;
					data += itemsize/4;
					delta->num_update_items++;
				}
				perf_end();
			}
			else
			{
				static PERFORMACE_INFO scope = {"copy", 0};
				perf_start(&scope);
				
				*data++ = snapitem_type(curitem);
				*data++ = snapitem_id(curitem);
				if(!item_sizes[snapitem_type(curitem)])
					*data++ = itemsize/4;
				
				mem_copy(data, snapitem_data(curitem), itemsize);
				size_count += itemsize;
				data += itemsize/4;
				delta->num_update_items++;
				count++;
				
				perf_end();
			}
		}
		
		perf_end();
	}
	
	if(0)
	{
		dbg_msg("snapshot", "%d %d %d",
			delta->num_deleted_items,
			delta->num_update_items,
			delta->num_temp_items);
	}

	/*
	// TODO: pack temp stuff
	
	// finish
	//mem_copy(delta->offsets, deleted, delta->num_deleted_items*sizeof(int));
	//mem_copy(&(delta->offsets[delta->num_deleted_items]), update, delta->num_update_items*sizeof(int));
	//mem_copy(&(delta->offsets[delta->num_deleted_items+delta->num_update_items]), temp, delta->num_temp_items*sizeof(int));
	//mem_copy(delta->data_start(), data, data_size);
	//delta->data_size = data_size;
	* */
	
	if(!delta->num_deleted_items && !delta->num_update_items && !delta->num_temp_items)
		return 0;
	
	return (int)((char*)data-(char*)dstdata);
}

static int range_check(void *end, void *ptr, int size)
{
	if((const char *)ptr + size > (const char *)end)
		return -1;
	return 0;
}

int snapshot_unpack_delta(SNAPSHOT *from, SNAPSHOT *to, void *srcdata, int data_size)
{
	SNAPBUILD builder;
	SNAPSHOT_DELTA *delta = (SNAPSHOT_DELTA *)srcdata;
	int *data = (int *)delta->data;
	int *end = (int *)(((char *)srcdata + data_size));
	
	SNAPSHOT_ITEM *fromitem;
	int i, d, keep, itemsize;
	int *deleted;
	int id, type, key;
	int fromindex;
	int *newdata;
			
	snapbuild_init(&builder);
	
	/* unpack deleted stuff */
	deleted = data;
	data += delta->num_deleted_items;
	if(data > end)
		return -1;

	/* copy all non deleted stuff */
	for(i = 0; i < from->num_items; i++)
	{
		/* dbg_assert(0, "fail!"); */
		fromitem = snapshot_get_item(from, i);
		itemsize = snapshot_get_item_datasize(from, i);
		keep = 1;
		for(d = 0; d < delta->num_deleted_items; d++)
		{
			if(deleted[d] == snapitem_key(fromitem))
			{
				keep = 0;
				break;
			}
		}
		
		if(keep)
		{
			/* keep it */
			mem_copy(
				snapbuild_new_item(&builder, snapitem_type(fromitem), snapitem_id(fromitem), itemsize),
				snapitem_data(fromitem), itemsize);
		}
	}
		
	/* unpack updated stuff */
	for(i = 0; i < delta->num_update_items; i++)
	{
		if(data+2 > end)
			return -1;
		
		type = *data++;
		id = *data++;
		if(item_sizes[type])
			itemsize = item_sizes[type];
		else
		{
			if(data+1 > end)
				return -2;
			itemsize = (*data++) * 4;
		}
		snapshot_current = type;
		
		if(range_check(end, data, itemsize) || itemsize < 0) return -3;
		
		key = (type<<16)|id;
		
		/* create the item if needed */
		newdata = snapbuild_get_item_data(&builder, key);
		if(!newdata)
			newdata = (int *)snapbuild_new_item(&builder, key>>16, key&0xffff, itemsize);

		/*if(range_check(end, newdata, itemsize)) return -4;*/
			
		fromindex = snapshot_get_item_index(from, key);
		if(fromindex != -1)
		{
			/* we got an update so we need to apply the diff */
			undiff_item((int *)snapitem_data(snapshot_get_item(from, fromindex)), data, newdata, itemsize/4);
			snapshot_data_updates[snapshot_current]++;
		}
		else /* no previous, just copy the data */
		{
			mem_copy(newdata, data, itemsize);
			snapshot_data_rate[snapshot_current] += itemsize*8;
			snapshot_data_updates[snapshot_current]++;
		}
			
		data += itemsize/4;
	}
	
	/* finish up */
	return snapbuild_finish(&builder, to);
}

/* SNAPSTORAGE */

void snapstorage_init(SNAPSTORAGE *ss)
{
	ss->first = 0;
}

void snapstorage_purge_all(SNAPSTORAGE *ss)
{
	SNAPSTORAGE_HOLDER *h = ss->first;
	SNAPSTORAGE_HOLDER *next;

	while(h)
	{
		next = h->next;
		mem_free(h);
		h = next;
	}

	/* no more snapshots in storage */
	ss->first = 0;
	ss->last = 0;
}

void snapstorage_purge_until(SNAPSTORAGE *ss, int tick)
{
	SNAPSTORAGE_HOLDER *next;
	SNAPSTORAGE_HOLDER *h = ss->first;
	
	while(h)
	{
		next = h->next;
		if(h->tick >=  tick)
			return; /* no more to remove */
		mem_free(h);
		
		/* did we come to the end of the list? */
        if (!next)
            break;

		ss->first = next;
		next->prev = 0x0;
		
		h = next;
	}
	
	/* no more snapshots in storage */
	ss->first = 0;
	ss->last = 0;
}

void snapstorage_add(SNAPSTORAGE *ss, int tick, int64 tagtime, int data_size, void *data, int create_alt)
{
	/* allocate memory for holder + snapshot_data */
	SNAPSTORAGE_HOLDER *h;
	int total_size = sizeof(SNAPSTORAGE_HOLDER)+data_size;
	
	if(create_alt)
		total_size += data_size;
	
	h = (SNAPSTORAGE_HOLDER *)mem_alloc(total_size, 1);
	
	/* set data */
	h->tick = tick;
	h->tagtime = tagtime;
	h->snap_size = data_size;
	h->snap = (SNAPSHOT*)(h+1);
	mem_copy(h->snap, data, data_size);

	if(create_alt) /* create alternative if wanted */
	{
		h->alt_snap = (SNAPSHOT*)(((char *)h->snap) + data_size);
		mem_copy(h->alt_snap, data, data_size);
	}
	else
		h->alt_snap = 0;
		
	
	/* link */
	h->next = 0;
	h->prev = ss->last;
	if(ss->last)
		ss->last->next = h;
	else
		ss->first = h;
	ss->last = h;
}

int snapstorage_get(SNAPSTORAGE *ss, int tick, int64 *tagtime, SNAPSHOT **data, SNAPSHOT **alt_data)
{
	SNAPSTORAGE_HOLDER *h = ss->first;
	
	while(h)
	{
		if(h->tick == tick)
		{
			if(tagtime)
				*tagtime = h->tagtime;
			if(data)
				*data = h->snap;
			if(alt_data)
				*alt_data = h->alt_snap;
			return h->snap_size;
		}
		
		h = h->next;
	}
	
	return -1;
}

/* SNAPBUILD */

void snapbuild_init(SNAPBUILD *sb)
{
	sb->data_size = 0;
	sb->num_items = 0;
}

SNAPSHOT_ITEM *snapbuild_get_item(SNAPBUILD *sb, int index) 
{
	return (SNAPSHOT_ITEM *)&(sb->data[sb->offsets[index]]);
}

int *snapbuild_get_item_data(SNAPBUILD *sb, int key)
{
	int i;
	for(i = 0; i < sb->num_items; i++)
	{
		if(snapitem_key(snapbuild_get_item(sb, i)) == key)
			return (int *)snapitem_data(snapbuild_get_item(sb, i));
	}
	return 0;
}

int snapbuild_finish(SNAPBUILD *sb, void *snapdata)
{
	/* flattern and make the snapshot */
	SNAPSHOT *snap = (SNAPSHOT *)snapdata;
	int offset_size = sizeof(int)*sb->num_items;
	snap->data_size = sb->data_size;
	snap->num_items = sb->num_items;
	mem_copy(snapshot_offsets(snap), sb->offsets, offset_size);
	mem_copy(snapshot_datastart(snap), sb->data, sb->data_size);
	return sizeof(SNAPSHOT) + offset_size + sb->data_size;
}

void *snapbuild_new_item(SNAPBUILD *sb, int type, int id, int size)
{
	SNAPSHOT_ITEM *obj = (SNAPSHOT_ITEM *)(sb->data+sb->data_size);

	/*if(stress_prob(0.01f))
	{
		size += ((rand()%5) - 2)*4;
		if(size < 0)
			size = 0;
	}*/

	mem_zero(obj, sizeof(SNAPSHOT_ITEM) + size);
	obj->type_and_id = (type<<16)|id;
	sb->offsets[sb->num_items] = sb->data_size;
	sb->data_size += sizeof(SNAPSHOT_ITEM) + size;
	sb->num_items++;
	
	dbg_assert(sb->data_size < MAX_SNAPSHOT_SIZE, "too much data");
	dbg_assert(sb->num_items < SNAPBUILD_MAX_ITEMS, "too many items");

	return snapitem_data(obj);
}
