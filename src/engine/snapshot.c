#include "snapshot.h"


int *snapitem_data(SNAPSHOT_ITEM *item) { return (int *)(item+1); }
int snapitem_type(SNAPSHOT_ITEM *item) { return item->type_and_id>>16; }
int snapitem_id(SNAPSHOT_ITEM *item) { return item->type_and_id&0xffff; }
int snapitem_key(SNAPSHOT_ITEM *item) { return item->type_and_id; }

int *snapshot_offsets(SNAPSHOT *snap) { return (int *)(snap+1); }
char *snapshot_datastart(SNAPSHOT *snap) { return (char*)(snapshot_offsets(snap)+snap->num_items); }

SNAPSHOT_ITEM *snapshot_get_item(SNAPSHOT *snap, int index)
{ return (SNAPSHOT_ITEM *)(snapshot_datastart(snap) + snapshot_offsets(snap)[index]); };

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
	/*
	int needed = 0;
	while(size)
	{
		*out = *current-*past;
		if(*out)
			needed = 1;
		out++;
		current++;
		past++;
		size--;
	}*/

	int needed = 0;
	while(size)
	{
		*out = *current-*past;
		if(*out)
			needed = 1;
		
		out++;
		past++;
		current++;
		size--;
	}
	
	return needed;
}

static void undiff_item(int *past, int *diff, int *out, int size)
{
	while(size)
	{
		*out = *past+*diff;
		out++;
		past++;
		diff++;
		size--;
	}
}


/* TODO: OPT: this should be made much faster */
int snapshot_create_delta(SNAPSHOT *from, SNAPSHOT *to, void *dstdata)
{
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

	/* pack deleted stuff */
	for(i = 0; i < from->num_items; i++)
	{
		fromitem = snapshot_get_item(from, i);
		if(snapshot_get_item_index(to, (snapitem_key(fromitem))) == -1)
		{
			/* deleted */
			delta->num_deleted_items++;
			*data = snapitem_key(fromitem);
			data++;
		}
	}
	
	/* pack updated stuff */
	for(i = 0; i < to->num_items; i++)
	{
		/* do delta */
		itemsize = snapshot_get_item_datasize(to, i);
		
		curitem = snapshot_get_item(to, i);
		pastindex = snapshot_get_item_index(from, snapitem_key(curitem));
		if(pastindex != -1)
		{
			pastitem = snapshot_get_item(from, pastindex);
			if(diff_item((int*)snapitem_data(pastitem), (int*)snapitem_data(curitem), data+3, itemsize/4))
			{
				*data++ = itemsize;
				*data++ = snapitem_type(curitem);
				*data++ = snapitem_id(curitem);
				/*data++ = curitem->key();*/
				data += itemsize/4;
				delta->num_update_items++;
			}
		}
		else
		{
			*data++ = itemsize;
			*data++ = snapitem_type(curitem);
			*data++ = snapitem_id(curitem);
			/*data++ = curitem->key();*/
			
			mem_copy(data, snapitem_data(curitem), itemsize);
			size_count += itemsize;
			data += itemsize/4;
			delta->num_update_items++;
			count++;
		}
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

int snapshot_unpack_delta(SNAPSHOT *from, SNAPSHOT *to, void *srcdata, int data_size)
{
	SNAPBUILD builder;
	SNAPSHOT_DELTA *delta = (SNAPSHOT_DELTA *)srcdata;
	int *data = (int *)delta->data;
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
		itemsize = *data++;
		type = *data++;
		id = *data++;
		
		key = (type<<16)|id;
		
		/* create the item if needed */
		newdata = snapbuild_get_item_data(&builder, key);
		if(!newdata)
			newdata = (int *)snapbuild_new_item(&builder, key>>16, key&0xffff, itemsize);
			
		fromindex = snapshot_get_item_index(from, key);
		if(fromindex != -1)
		{
			/* we got an update so we need to apply the diff */
			undiff_item((int *)snapitem_data(snapshot_get_item(from, fromindex)), data, newdata, itemsize/4);
		}
		else /* no previous, just copy the data */
			mem_copy(newdata, data, itemsize);
			
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
		
		ss->first = next;
		next->prev = 0x0;
		
		h = next;
	}
	
	/* no more snapshots in storage */
	ss->first = 0;
	ss->last = 0;
}

void snapstorage_add(SNAPSTORAGE *ss, int tick, int64 tagtime, int data_size, void *data)
{
	/* allocate memory for holder + snapshot_data */
	SNAPSTORAGE_HOLDER *h = (SNAPSTORAGE_HOLDER *)mem_alloc(sizeof(SNAPSTORAGE_HOLDER)+data_size, 1);
	
	/* set data */
	h->tick = tick;
	h->tagtime = tagtime;
	h->snap_size = data_size;
	h->snap = (SNAPSHOT*)(h+1);
	mem_copy(h->snap, data, data_size);
	
	/* link */
	h->next = 0;
	h->prev = ss->last;
	if(ss->last)
		ss->last->next = h;
	else
		ss->first = h;
	ss->last = h;
}

int snapstorage_get(SNAPSTORAGE *ss, int tick, int64 *tagtime, SNAPSHOT **data)
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
	snap->data_size = sb->data_size;
	snap->num_items = sb->num_items;
	int offset_size = sizeof(int)*sb->num_items;
	mem_copy(snapshot_offsets(snap), sb->offsets, offset_size);
	mem_copy(snapshot_datastart(snap), sb->data, sb->data_size);
	return sizeof(SNAPSHOT) + offset_size + sb->data_size;
}

void *snapbuild_new_item(SNAPBUILD *sb, int type, int id, int size)
{
	SNAPSHOT_ITEM *obj = (SNAPSHOT_ITEM *)(sb->data+sb->data_size);
	mem_zero(obj, sizeof(SNAPSHOT_ITEM) + size);
	obj->type_and_id = (type<<16)|id;
	sb->offsets[sb->num_items] = sb->data_size;
	sb->data_size += sizeof(SNAPSHOT_ITEM) + size;
	sb->num_items++;
	
	dbg_assert(sb->data_size < MAX_SNAPSHOT_SIZE, "too much data");
	dbg_assert(sb->num_items < SNAPBUILD_MAX_ITEMS, "too many items");

	return snapitem_data(obj);
}
