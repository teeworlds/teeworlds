#include <baselib/system.h>
#include <baselib/stream/file.h>

#include "datafile.h"

static const int DEBUG=0;

struct item_type
{
	int type;
	int start;
	int num;
};

struct item
{
	int type_and_id;
	int size;
};

struct datafile_header
{
	int id;
	int version;
	int size;
	int swaplen;
	int num_item_types;
	int num_items;
	int num_raw_data;
	int item_size;
	int data_size;
};

struct datafile_data
{
	int num_item_types;
	int num_items;
	int num_raw_data;
	int item_size;
	int data_size;
	char start[4];
};

struct datafile_info
{
	item_type *item_types;
	int *item_offsets;
	int *data_offsets;

	char *item_start;
	char *data_start;	
};

struct datafile
{
	datafile_info info;
	datafile_data data;
};

datafile *datafile_load(const char *filename)
{
	dbg_msg("datafile", "datafile loading. filename='%s'", filename);

	baselib::file_stream file;
	if(!file.open_r(filename))
		return 0;
	
	// TODO: change this header
	int header[4];
	file.read(header, sizeof(header));
	if(((header[0]>>24)&0xff) != 'D' || ((header[0]>>16)&0xff) != 'A' || (header[0]>>8)&0xff != 'T' || (header[0]&0xff)!= 'A')
	{
		dbg_msg("datafile", "wrong signature. %x %x %x %x", header[0], header[1], header[2], header[3]);
		return 0;
	}
	
	int version = header[1];
	if(version != 3)
	{
		dbg_msg("datafile", "wrong version. version=%x", version);
		return 0;
	}
	
	unsigned size = header[2];
	unsigned swapsize = header[3];
	
	if(DEBUG)
		dbg_msg("datafile", "loading. size=%d", size);
	
	// TODO: use this variable for good and awesome
	(void)swapsize;
	
	//map_unload();
	datafile *df = (datafile*)mem_alloc(size+sizeof(datafile_info), 1);
	unsigned readsize = file.read(&df->data, size);
	if(readsize != size)
	{
		dbg_msg("datafile", "couldn't load the whole thing, wanted=%d got=%d", size, readsize);
		return 0;
	}
	
	// TODO: byteswap
	//map->byteswap();

	if(DEBUG)
		dbg_msg("datafile", "item_size=%d", df->data.item_size);
	
	
	df->info.item_types = (item_type *)df->data.start;
	df->info.item_offsets = (int *)&df->info.item_types[df->data.num_item_types];
	df->info.data_offsets = (int *)&df->info.item_offsets[df->data.num_items];
	
	df->info.item_start = (char *)&df->info.data_offsets[df->data.num_raw_data];
	df->info.data_start = df->info.item_start + df->data.item_size;

	if(DEBUG)
		dbg_msg("datafile", "datafile loading done. datafile='%s'", filename);

	if(DEBUG)
	{
		for(int i = 0; i < df->data.num_raw_data; i++)
		{
			void *p = datafile_get_data(df, i);
			dbg_msg("datafile", "%d %d", (int)((char*)p - (char*)(&df->data)), size);
		}
			
		for(int i = 0; i < datafile_num_items(df); i++)
		{
			int type, id;
			void *data = datafile_get_item(df, i, &type, &id);
			dbg_msg("map", "\t%d: type=%x id=%x p=%p offset=%d", i, type, id, data, df->info.item_offsets[i]);
			int *idata = (int*)data;
			for(int k = 0; k < 3; k++)
				dbg_msg("datafile", "\t\t%d=%d (%x)", k, idata[k], idata[k]);
		}

		for(int i = 0; i < df->data.num_item_types; i++)
		{
			dbg_msg("map", "\t%d: type=%x start=%d num=%d", i,
				df->info.item_types[i].type,
				df->info.item_types[i].start,
				df->info.item_types[i].num);
			for(int k = 0; k < df->info.item_types[i].num; k++)
			{
				int type, id;
				datafile_get_item(df, df->info.item_types[i].start+k, &type, &id);
				if(type != df->info.item_types[i].type)
					dbg_msg("map", "\tERROR");
			}
		}
	}
		
	return df;
}

void *datafile_get_data(datafile *df, int index)
{
	return df->info.data_start+df->info.data_offsets[index];
}

void *datafile_get_item(datafile *df, int index, int *type, int *id)
{
	item *i = (item *)(df->info.item_start+df->info.item_offsets[index]);
	if(type)
		*type = (i->type_and_id>>16)&0xffff; // remove sign extention
	if(id)
		*id = i->type_and_id&0xffff;
	return (void *)(i+1);
}

void datafile_get_type(datafile *df, int type, int *start, int *num)
{
	for(int i = 0; i < df->data.num_item_types; i++)
	{
		if(df->info.item_types[i].type == type)
		{
			*start = df->info.item_types[i].start;
			*num = df->info.item_types[i].num;
			return;
		}
	}
	
	*start = 0;
	*num = 0;
}

void *datafile_find_item(datafile *df, int type, int id)
{
	int start, num;
	datafile_get_type(df, type, &start, &num);
	for(int i = 0; i < num; i++)
	{
		int item_id;
		void *item = datafile_get_item(df, start+i,0, &item_id);
		if(id == item_id)
			return item;
	}
	return 0;
}

int datafile_num_items(datafile *df)
{
	return df->data.num_items;
}

void datafile_unload(datafile *df)
{
	if(df)
		mem_free(df);
}

// DATAFILE output
struct data_info
{
	int size;
	void *data;
};

struct item_info
{
	int type;
	int id;
	int size;
	int next;
	int prev;
	void *data;
};

struct itemtype_info
{
	int num;
	int first;
	int last;
};

//
struct datafile_out
{
	baselib::file_stream file;
	int num_items;
	int num_datas;
	int num_item_types;
	itemtype_info item_types[0xffff];
	item_info items[1024];
	data_info datas[1024];
};

datafile_out *datafile_create(const char *filename)
{
	datafile_out *df = new datafile_out;
	if(!df->file.open_w(filename))
	{
		delete df;
		return 0;
	}
	
	df->num_items = 0;
	df->num_datas = 0;
	df->num_item_types = 0;
	mem_zero(&df->item_types, sizeof(df->item_types));

	for(int i = 0; i < 0xffff; i++)
	{
		df->item_types[i].first = -1;
		df->item_types[i].last = -1;
	}
	
	return df;
}

int datafile_add_item(datafile_out *df, int type, int id, int size, void *data)
{
	df->items[df->num_items].type = type;
	df->items[df->num_items].id = id;
	df->items[df->num_items].size = size;
	
	// copy data
	df->items[df->num_items].data = mem_alloc(size, 1);
	mem_copy(df->items[df->num_items].data, data, size);

	if(!df->item_types[type].num) // count item types
		df->num_item_types++;

	// link
	df->items[df->num_items].prev = df->item_types[type].last;
	df->items[df->num_items].next = -1;
	
	if(df->item_types[type].last != -1)
		df->items[df->item_types[type].last].next = df->num_items;
	df->item_types[type].last = df->num_items;
	
	if(df->item_types[type].first == -1)
		df->item_types[type].first = df->num_items;
	
	df->item_types[type].num++;
		
	df->num_items++;
	return df->num_items-1;
}

int datafile_add_data(datafile_out *df, int size, void *data)
{
	df->datas[df->num_items].size = size;
	df->datas[df->num_datas].data = data;
	df->num_datas++;
	return df->num_datas-1;
}

int datafile_finish(datafile_out *df)
{
	// we should now write this file!
	if(DEBUG)
		dbg_msg("datafile", "writing");

	// calculate sizes
	int itemsize = 0;
	for(int i = 0; i < df->num_items; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "item=%d size=%d (%d)", i, df->items[i].size, df->items[i].size+sizeof(item));
		itemsize += df->items[i].size + sizeof(item);
	}
	
	int datasize = 0;
	for(int i = 0; i < df->num_datas; i++)
		datasize += df->datas[i].size;
	
	// calculate the complete size
	int typessize = df->num_item_types*sizeof(item_type);
	int headersize = sizeof(datafile_header);
	int offsetsize = df->num_items*sizeof(int) + df->num_datas*sizeof(int);
	int filesize = headersize + typessize + offsetsize + itemsize + datasize;
	int swapsize = filesize - datasize;
	
	if(DEBUG)
		dbg_msg("datafile", "num_item_types=%d typessize=%d itemsize=%d datasize=%d", df->num_item_types, typessize, itemsize, datasize);
	
	// construct header
	datafile_header header;
	header.id = ('D'<<24) | ('A'<<16) | ('T'<<8) | ('A');
	header.version = 3;
	header.size = filesize - 16;
	header.swaplen = swapsize - 16;
	header.num_item_types = df->num_item_types;
	header.num_items = df->num_items;
	header.num_raw_data = df->num_datas;
	header.item_size = itemsize;
	header.data_size = datasize;
	
	// TODO: apply swapping
	// write header
	if(DEBUG)
		dbg_msg("datafile", "headersize=%d", sizeof(header));
	df->file.write(&header, sizeof(header));
	
	// write types
	for(int i = 0, count = 0; i < 0xffff; i++)
	{
		if(df->item_types[i].num)
		{
			// write info
			item_type info;
			info.type = i;
			info.start = count;
			info.num = df->item_types[i].num;
			if(DEBUG)
				dbg_msg("datafile", "writing type=%x start=%d num=%d", info.type, info.start, info.num);
			df->file.write(&info, sizeof(info));
			
			count += df->item_types[i].num;
		}
	}
	
	// write item offsets
	for(int i = 0, offset = 0; i < 0xffff; i++)
	{
		if(df->item_types[i].num)
		{
			// write all items in of this type
			int k = df->item_types[i].first;
			while(k != -1)
			{
				if(DEBUG)
					dbg_msg("datafile", "writing item offset num=%d offset=%d", k, offset);
				df->file.write(&offset, sizeof(offset));
				offset += df->items[k].size + sizeof(item);
				
				// next
				k = df->items[k].next;
			}
		}
	}
	
	// write data offsets
	for(int i = 0, offset = 0; i < df->num_datas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data offset num=%d offset=%d", i, offset);
		df->file.write(&offset, sizeof(offset));
		offset += df->datas[i].size;
	}
	
	// write items
	for(int i = 0; i < 0xffff; i++)
	{
		if(df->item_types[i].num)
		{
			// write all items in of this type
			int k = df->item_types[i].first;
			while(k != -1)
			{
				item itm;
				itm.type_and_id = (i<<16)|df->items[k].id;
				itm.size = df->items[k].size;
				if(DEBUG)
					dbg_msg("datafile", "writing item type=%x idx=%d id=%d size=%d", i, k, df->items[k].id, df->items[k].size);
				df->file.write(&itm, sizeof(itm));
				df->file.write(df->items[k].data, df->items[k].size);
				
				// next
				k = df->items[k].next;
			}
		}
	}
	
	// write data
	for(int i = 0; i < df->num_datas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data id=%d size=%d", i, df->datas[i].size);
		df->file.write(df->datas[i].data, df->datas[i].size);
	}

	// free data
	for(int i = 0; i < df->num_items; i++)
		mem_free(df->items[i].data);

	
	delete df;

	if(DEBUG)
		dbg_msg("datafile", "done");
	return 0;
}
