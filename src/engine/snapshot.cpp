
#include "packet.h"
#include "snapshot.h"

struct snapshot_delta
{
	int num_deleted_items;
	int num_update_items;
	int num_temp_items; // needed?
	int data[1];
	
	/*
	char *data_start() { return (char *)&offsets[num_deleted_items+num_update_items+num_temp_items]; }
	
	int deleted_item(int index) { return offsets[index]; }
	item *update_item(int index) { return (item *)(data_start() + offsets[num_deleted_items+index]); }
	item *temp_item(int index) { return (item *)(data_start() + offsets[num_deleted_items+num_update_items+index]); }
	* */
};


static const int MAX_ITEMS = 512;
static snapshot_delta empty = {0,0,0,{0}};

void *snapshot_empty_delta()
{
	return &empty;
}

int snapshot_crc(snapshot *snap)
{
	int crc = 0;
	
	for(int i = 0; i < snap->num_items; i++)
	{
		snapshot::item *item = snap->get_item(i);
		int size = snap->get_item_datasize(i);
		
		for(int b = 0; b < size/4; b++)
			crc += item->data()[b];
	}
	return crc;
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

// 1 = 4-3
// d = n-p

// n(4) = p(3)+d(1)

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

int snapshot_create_delta(snapshot *from, snapshot *to, void *dstdata)
{
	//int deleted[MAX_ITEMS];
	//int update[MAX_ITEMS];
	//int mark[MAX_ITEMS];
	//char data[MAX_SNAPSHOT_SIZE];
	
	snapshot_delta *delta = (snapshot_delta *)dstdata;
	int *data = (int *)delta->data;
	
	
	delta->num_deleted_items = 0;
	delta->num_update_items = 0;
	delta->num_temp_items = 0;

	// pack deleted stuff
	for(int i = 0; i < from->num_items; i++)
	{
		snapshot::item *fromitem = from->get_item(i);
		if(to->get_item_index(fromitem->key()) == -1)
		{
			// deleted
			delta->num_deleted_items++;
			*data = fromitem->key();
			data++;
		}
	}
	
	// pack updated stuff
	int count = 0, size_count = 0;
	for(int i = 0; i < to->num_items; i++)
	{
		// do delta
		int itemsize = to->get_item_datasize(i);
		
		snapshot::item *curitem = to->get_item(i);
		int pastindex = from->get_item_index(curitem->key());
		if(pastindex != -1)
		{
			snapshot::item *pastitem = from->get_item(pastindex);
			if(diff_item((int*)pastitem->data(), (int*)curitem->data(), data+3, itemsize/4))
			{
				*data++ = itemsize;
				*data++ = curitem->type();
				*data++ = curitem->id();
				//*data++ = curitem->key();
				data += itemsize/4;
				delta->num_update_items++;
			}
		}
		else
		{
			*data++ = itemsize;
			*data++ = curitem->type();
			*data++ = curitem->id();
			//*data++ = curitem->key();
			
			mem_copy(data, curitem->data(), itemsize);
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

	// TODO: pack temp stuff
	
	// finish
	//mem_copy(delta->offsets, deleted, delta->num_deleted_items*sizeof(int));
	//mem_copy(&(delta->offsets[delta->num_deleted_items]), update, delta->num_update_items*sizeof(int));
	//mem_copy(&(delta->offsets[delta->num_deleted_items+delta->num_update_items]), temp, delta->num_temp_items*sizeof(int));
	//mem_copy(delta->data_start(), data, data_size);
	//delta->data_size = data_size;
	
	if(!delta->num_deleted_items && !delta->num_update_items && !delta->num_temp_items)
		return 0;
	
	return (int)((char*)data-(char*)dstdata);
}

int snapshot_unpack_delta(snapshot *from, snapshot *to, void *srcdata, int data_size)
{
	snapshot_builder builder;
	snapshot_delta *delta = (snapshot_delta *)srcdata;
	int *data = (int *)delta->data;
	
	builder.start();
	
	// unpack deleted stuff
	int *deleted = data;
	data += delta->num_deleted_items;

	// copy all non deleted stuff
	for(int i = 0; i < from->num_items; i++)
	{
		//dbg_assert(0, "fail!");
		snapshot::item *fromitem = from->get_item(i);
		int itemsize = from->get_item_datasize(i);
		int keep = 1;
		for(int d = 0; d < delta->num_deleted_items; d++)
		{
			if(deleted[d] == fromitem->key())
			{
				keep = 0;
				break;
			}
		}
		
		if(keep)
		{
			// keep it
			int *newdata = (int *)(snapshot::item *)builder.new_item(fromitem->type(), fromitem->id(), itemsize);
			mem_copy(newdata, fromitem->data(), itemsize);
		}
	}
		
	// unpack updated stuff
	for(int i = 0; i < delta->num_update_items; i++)
	{
		int itemsize, id, type, key;
		itemsize = *data++;
		//key = *data++;
		type = *data++;
		id = *data++;
		
		key = (type<<16)|id;
		
		// create the item if needed
		int *newdata = builder.get_item_data(key);
		if(!newdata)
			newdata = (int *)builder.new_item(key>>16, key&0xffff, itemsize);
			
		int fromindex = from->get_item_index(key);
		if(fromindex != -1)
		{
			// we got an update so we need to apply the diff
			int *pastdata = (int *)from->get_item(fromindex)->data();
			undiff_item(pastdata, data, newdata, itemsize/4);
		}
		else // no previous, just copy the data
			mem_copy(newdata, data, itemsize);
			
		data += itemsize/4;
	}
	
	// TODO: unpack temp stuff
	
	// finish up
	return builder.finish(to);
}
