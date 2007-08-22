
#include "system.h"
#include "datafile.h"

static const int DEBUG=0;

typedef struct 
{
	int type;
	int start;
	int num;
} DATAFILE_ITEM_TYPE;

typedef struct 
{
	int type_and_id;
	int size;
} DATAFILE_ITEM;

typedef struct
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
} DATAFILE_HEADER;

typedef struct
{
	int num_item_types;
	int num_items;
	int num_raw_data;
	int item_size;
	int data_size;
	char start[4];
} DATAFILE_DATA;

typedef struct
{
	DATAFILE_ITEM_TYPE *item_types;
	int *item_offsets;
	int *data_offsets;

	char *item_start;
	char *data_start;	
} DATAFILE_INFO;

struct DATAFILE_t
{
	DATAFILE_INFO info;
	DATAFILE_DATA data;
};

DATAFILE *datafile_load(const char *filename)
{
	DATAFILE *df;
	IOHANDLE file;
	unsigned char header[16];
	int version;
	unsigned size, swapsize, readsize;
	
	unsigned *dst;
	unsigned char *src;
	unsigned j;
	(void)dst;
	(void)src;
	(void)j;
	
	
	dbg_msg("datafile", "datafile loading. filename='%s'", filename);

	file = io_open(filename, IOFLAG_READ);
	if(!file)
		return 0;
	
	/* TODO: change this header */
	io_read(file, header, sizeof(header));
	if(header[3] != 'D' || header[2] != 'A' || header[1] != 'T' || header[0] != 'A')
	{
		dbg_msg("datafile", "wrong signature. %x %x %x %x", header[0], header[1], header[2], header[3]);
		return 0;
	}
	
	version = (unsigned)header[4] | (unsigned)header[5]<<8 | (unsigned)header[6]<<16 | (unsigned)header[7]<<24;
	if(version != 3)
	{
		dbg_msg("datafile", "wrong version. version=%x", version);
		return 0;
	}
	
	size = (unsigned)header[8] | (unsigned)header[9]<<8 | (unsigned)header[10]<<16 | (unsigned)header[11]<<24;
	swapsize = (unsigned)header[12] | (unsigned)header[13]<<8 | (unsigned)header[14]<<16 | (unsigned)header[15]<<24;
	
	(void)swapsize;
	
	if(DEBUG)
		dbg_msg("datafile", "loading. size=%d", size);
	
	df = (DATAFILE*)mem_alloc(size+sizeof(DATAFILE_INFO), 1);
	readsize = io_read(file, &df->data, size);
	if(readsize != size)
	{
		dbg_msg("datafile", "couldn't load the whole thing, wanted=%d got=%d", size, readsize);
		return 0;
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	unsigned *dst = (unsigned*)df;
	unsigned char *src = (unsigned char*)df;
	for(unsigned i = 0; i < swapsize; i++)
	{
		unsigned j = i << 2;
		dst[i] = src[j] | src[j+1]<<8 | src[j+2]<<16 | src[j+3]<<24;
	}
#endif
	
	if(DEBUG)
		dbg_msg("datafile", "item_size=%d", df->data.item_size);
	
	df->info.item_types = (DATAFILE_ITEM_TYPE *)df->data.start;
	df->info.item_offsets = (int *)&df->info.item_types[df->data.num_item_types];
	df->info.data_offsets = (int *)&df->info.item_offsets[df->data.num_items];
	
	df->info.item_start = (char *)&df->info.data_offsets[df->data.num_raw_data];
	df->info.data_start = df->info.item_start + df->data.item_size;

	if(DEBUG)
		dbg_msg("datafile", "datafile loading done. datafile='%s'", filename);

	if(DEBUG)
	{
		/*
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
		*/
	}
		
	return df;
}

void *datafile_get_data(DATAFILE *df, int index)
{
	return df->info.data_start+df->info.data_offsets[index];
}

void *datafile_get_item(DATAFILE *df, int index, int *type, int *id)
{
	DATAFILE_ITEM *i = (DATAFILE_ITEM *)(df->info.item_start+df->info.item_offsets[index]);
	if(type)
		*type = (i->type_and_id>>16)&0xffff; /* remove sign extention */
	if(id)
		*id = i->type_and_id&0xffff;
	return (void *)(i+1);
}

void datafile_get_type(DATAFILE *df, int type, int *start, int *num)
{
	int i;
	for(i = 0; i < df->data.num_item_types; i++)
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

void *datafile_find_item(DATAFILE *df, int type, int id)
{
	int start, num,i ;
	int item_id;
	void *item;
	
	datafile_get_type(df, type, &start, &num);
	for(i = 0; i < num; i++)
	{
		
		item = datafile_get_item(df, start+i,0, &item_id);
		if(id == item_id)
			return item;
	}
	return 0;
}

int datafile_num_items(DATAFILE *df)
{
	return df->data.num_items;
}

void datafile_unload(DATAFILE *df)
{
	if(df)
		mem_free(df);
}

/* DATAFILE output */
typedef struct
{
	int size;
	void *data;
} DATA_INFO;

typedef struct
{
	int type;
	int id;
	int size;
	int next;
	int prev;
	void *data;
} ITEM_INFO;

typedef struct
{
	int num;
	int first;
	int last;
} ITEMTYPE_INFO;

/* */
struct DATAFILE_OUT_t
{
	IOHANDLE file;
	int num_items;
	int num_datas;
	int num_item_types;
	ITEMTYPE_INFO item_types[0xffff];
	ITEM_INFO items[1024];
	DATA_INFO datas[1024];
};

DATAFILE_OUT *datafile_create(const char *filename)
{
	int i;
	DATAFILE_OUT *df = mem_alloc(sizeof(DATAFILE_OUT), 1);
	df->file = io_open(filename, IOFLAG_WRITE);
	if(!df->file)
	{
		mem_free(df);
		return 0;
	}
	
	df->num_items = 0;
	df->num_datas = 0;
	df->num_item_types = 0;
	mem_zero(&df->item_types, sizeof(df->item_types));

	for(i = 0; i < 0xffff; i++)
	{
		df->item_types[i].first = -1;
		df->item_types[i].last = -1;
	}
	
	return df;
}

int datafile_add_item(DATAFILE_OUT *df, int type, int id, int size, void *data)
{
	df->items[df->num_items].type = type;
	df->items[df->num_items].id = id;
	df->items[df->num_items].size = size;
	
	/* copy data */
	df->items[df->num_items].data = mem_alloc(size, 1);
	mem_copy(df->items[df->num_items].data, data, size);

	if(!df->item_types[type].num) /* count item types */
		df->num_item_types++;

	/* link */
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

int datafile_add_data(DATAFILE_OUT *df, int size, void *data)
{
	df->datas[df->num_items].size = size;
	df->datas[df->num_datas].data = data;
	df->num_datas++;
	return df->num_datas-1;
}

int datafile_finish(DATAFILE_OUT *df)
{
	/* we should now write this file! */
	if(DEBUG)
		dbg_msg("datafile", "writing");

	int itemsize = 0;
	int i, count, offset;
	int typessize, headersize, offsetsize, filesize, swapsize;
	int datasize = 0;
	DATAFILE_ITEM_TYPE info;
	DATAFILE_ITEM itm;
	
	/* calculate sizes */
	for(i = 0; i < df->num_items; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "item=%d size=%d (%d)", i, df->items[i].size, df->items[i].size+sizeof(DATAFILE_ITEM));
		itemsize += df->items[i].size + sizeof(DATAFILE_ITEM);
	}
	
	
	for(i = 0; i < df->num_datas; i++)
		datasize += df->datas[i].size;
	
	/* calculate the complete size */
	typessize = df->num_item_types*sizeof(DATAFILE_ITEM_TYPE);
	headersize = sizeof(DATAFILE_HEADER);
	offsetsize = df->num_items*sizeof(int) + df->num_datas*sizeof(int);
	filesize = headersize + typessize + offsetsize + itemsize + datasize;
	swapsize = filesize - datasize;
	
	(void)swapsize;
	
	if(DEBUG)
		dbg_msg("datafile", "num_item_types=%d typessize=%d itemsize=%d datasize=%d", df->num_item_types, typessize, itemsize, datasize);
	
	/* construct header */
	DATAFILE_HEADER header;
	header.id = ('D'<<24) | ('A'<<16) | ('T'<<8) | ('A');
	header.version = 3;
	header.size = filesize - 16;
	header.swaplen = swapsize - 16;
	header.num_item_types = df->num_item_types;
	header.num_items = df->num_items;
	header.num_raw_data = df->num_datas;
	header.item_size = itemsize;
	header.data_size = datasize;
	
	/* TODO: apply swapping */
	/* write header */
	if(DEBUG)
		dbg_msg("datafile", "headersize=%d", sizeof(header));
	io_write(df->file, &header, sizeof(header));
	
	/* write types */
	for(i = 0, count = 0; i < 0xffff; i++)
	{
		if(df->item_types[i].num)
		{
			/* write info */
			info.type = i;
			info.start = count;
			info.num = df->item_types[i].num;
			if(DEBUG)
				dbg_msg("datafile", "writing type=%x start=%d num=%d", info.type, info.start, info.num);
			io_write(df->file, &info, sizeof(info));
			count += df->item_types[i].num;
		}
	}
	
	/* write item offsets */
	for(i = 0, offset = 0; i < 0xffff; i++)
	{
		if(df->item_types[i].num)
		{
			/* write all items in of this type */
			int k = df->item_types[i].first;
			while(k != -1)
			{
				if(DEBUG)
					dbg_msg("datafile", "writing item offset num=%d offset=%d", k, offset);
				io_write(df->file, &offset, sizeof(offset));
				offset += df->items[k].size + sizeof(DATAFILE_ITEM);
				
				/* next */
				k = df->items[k].next;
			}
		}
	}
	
	/* write data offsets */
	for(i = 0, offset = 0; i < df->num_datas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data offset num=%d offset=%d", i, offset);
		io_write(df->file, &offset, sizeof(offset));
		offset += df->datas[i].size;
	}
	
	/* write items */
	for(i = 0; i < 0xffff; i++)
	{
		if(df->item_types[i].num)
		{
			/* write all items in of this type */
			int k = df->item_types[i].first;
			while(k != -1)
			{
				itm.type_and_id = (i<<16)|df->items[k].id;
				itm.size = df->items[k].size;
				if(DEBUG)
					dbg_msg("datafile", "writing item type=%x idx=%d id=%d size=%d", i, k, df->items[k].id, df->items[k].size);
				
				io_write(df->file, &itm, sizeof(itm));
				io_write(df->file, df->items[k].data, df->items[k].size);
				
				/* next */
				k = df->items[k].next;
			}
		}
	}
	
	/* write data */
	for(i = 0; i < df->num_datas; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "writing data id=%d size=%d", i, df->datas[i].size);
		io_write(df->file, df->datas[i].data, df->datas[i].size);
	}

	/* free data */
	for(i = 0; i < df->num_items; i++)
		mem_free(df->items[i].data);

	
	io_close(df->file);
	mem_free(df);

	if(DEBUG)
		dbg_msg("datafile", "done");
	return 0;
}
