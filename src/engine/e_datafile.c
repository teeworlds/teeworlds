/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "e_system.h"
#include "e_datafile.h"
#include <zlib.h>

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
	char id[4];
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
	int *data_sizes;

	char *item_start;
	char *data_start;
} DATAFILE_INFO;

struct DATAFILE_t
{
	IOHANDLE file;
	DATAFILE_INFO info;
	DATAFILE_HEADER header;
	int data_start_offset;
	char **data_ptrs;
	char *data;
};

DATAFILE *datafile_load(const char *filename)
{
	DATAFILE *df;
	IOHANDLE file;
	DATAFILE_HEADER header;
	unsigned readsize;
	
	unsigned *dst;
	unsigned char *src;
	unsigned j;
	int size = 0;
	int allocsize = 0;
	
	(void)dst;
	(void)src;
	(void)j;
	
	
	dbg_msg("datafile", "datafile loading. filename='%s'", filename);

	file = io_open(filename, IOFLAG_READ);
	if(!file)
		return 0;
	
	/* TODO: change this header */
	io_read(file, &header, sizeof(header));
	if(header.id[0] != 'A' || header.id[1] != 'T' || header.id[2] != 'A' || header.id[3] != 'D')
	{
		if(header.id[0] != 'D' || header.id[1] != 'A' || header.id[2] != 'T' || header.id[3] != 'A')
		{
			dbg_msg("datafile", "wrong signature. %x %x %x %x", header.id[0], header.id[1], header.id[2], header.id[3]);
			return 0;
		}
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(&header, sizeof(int), sizeof(header)/sizeof(int));	
#endif
	if(header.version != 3 && header.version != 4)
	{
		dbg_msg("datafile", "wrong version. version=%x", header.version);
		return 0;
	}
	
	/* read in the rest except the data */
	size = 0;
	size += header.num_item_types*sizeof(DATAFILE_ITEM_TYPE);
	size += (header.num_items+header.num_raw_data)*sizeof(int);
	if(header.version == 4)
		size += header.num_raw_data*sizeof(int); /* v4 has uncompressed data sizes aswell */
	size += header.item_size;
	
	allocsize = size;
	allocsize += sizeof(DATAFILE); /* add space for info structure */
	allocsize += header.num_raw_data*sizeof(void*); /* add space for data pointers */

	df = (DATAFILE*)mem_alloc(allocsize, 1);
	df->header = header;
	df->data_start_offset = sizeof(DATAFILE_HEADER) + size;
	df->data_ptrs = (char**)(df+1);
	df->data = (char *)(df+1)+header.num_raw_data*sizeof(char *);
	df->file = file;
	
	/* clear the data pointers */
	mem_zero(df->data_ptrs, header.num_raw_data*sizeof(void*));
	
	/* read types, offsets, sizes and item data */
	readsize = io_read(file, df->data, size);
	if(readsize != size)
	{
		dbg_msg("datafile", "couldn't load the whole thing, wanted=%d got=%d", size, readsize);
		return 0;
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(df->data, sizeof(int), header.swaplen);
#endif
	
	if(DEBUG)
		dbg_msg("datafile", "item_size=%d", df->header.item_size);
	
	df->info.item_types = (DATAFILE_ITEM_TYPE *)df->data;
	df->info.item_offsets = (int *)&df->info.item_types[df->header.num_item_types];
	df->info.data_offsets = (int *)&df->info.item_offsets[df->header.num_items];
	df->info.data_sizes = (int *)&df->info.data_offsets[df->header.num_raw_data];
	
	if(header.version == 4)
		df->info.item_start = (char *)&df->info.data_sizes[df->header.num_raw_data];
	else
		df->info.item_start = (char *)&df->info.data_offsets[df->header.num_raw_data];
	df->info.data_start = df->info.item_start + df->header.item_size;

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

int datafile_num_data(DATAFILE *df)
{
	return df->header.num_raw_data;
}

/* always returns the size in the file */
int datafile_get_datasize(DATAFILE *df, int index)
{
	if(index == df->header.num_raw_data-1)
		return df->header.data_size-df->info.data_offsets[index];
	return  df->info.data_offsets[index+1]-df->info.data_offsets[index];
}

void *datafile_get_data(DATAFILE *df, int index)
{
	/* load it if needed */
	if(!df->data_ptrs[index])
	{
		/* fetch the data size */
		int datasize = datafile_get_datasize(df, index);
		
		if(df->header.version == 4)
		{
			/* v4 has compressed data */
			void *temp = (char *)mem_alloc(datasize, 1);
			unsigned long uncompressed_size = df->info.data_sizes[index];
			unsigned long s;

			dbg_msg("datafile", "loading data index=%d size=%d", index, datasize);
			df->data_ptrs[index] = (char *)mem_alloc(uncompressed_size, 1);
			
			/* read the compressed data */
			io_seek(df->file, df->data_start_offset+df->info.data_offsets[index], IOSEEK_START);
			io_read(df->file, temp, datasize);
			
			/* decompress the data, TODO: check for errors */
			s = uncompressed_size;
			uncompress((Bytef*)df->data_ptrs[index], &s, (Bytef*)temp, datasize);
			
			/* clean up the temporary buffers */
			mem_free(temp);
		}
		else
		{
			/* load the data */
			dbg_msg("datafile", "loading data index=%d size=%d", index, datasize);
			df->data_ptrs[index] = (char *)mem_alloc(datasize, 1);
			io_seek(df->file, df->data_start_offset+df->info.data_offsets[index], IOSEEK_START);
			io_read(df->file, df->data_ptrs[index], datasize);
		}
	}
	
	return df->data_ptrs[index];
}

void *datafile_get_data_swapped(DATAFILE *df, int index)
{
	void *ptr = datafile_get_data(df, index);
	int size = datafile_get_datasize(df, index);
	(void)size; /* not used on LE machines */
	
	if(!ptr)
		return ptr;

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(ptr, sizeof(int), size);
#endif

	return ptr;
}


void datafile_unload_data(DATAFILE *df, int index)
{
	if(index < 0)
		return;
		
	/* */
	mem_free(df->data_ptrs[index]);
	df->data_ptrs[index] = 0x0;
}

int datafile_get_itemsize(DATAFILE *df, int index)
{
	if(index == df->header.num_items-1)
		return df->header.item_size-df->info.item_offsets[index];
	return  df->info.item_offsets[index+1]-df->info.item_offsets[index];
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
	for(i = 0; i < df->header.num_item_types; i++)
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
	return df->header.num_items;
}

void datafile_unload(DATAFILE *df)
{
	if(df)
	{
		/* free the data that is loaded */
		int i;
		for(i = 0; i < df->header.num_raw_data; i++)
			mem_free(df->data_ptrs[i]);
		
		io_close(df->file);
		mem_free(df);
	}
}

/* DATAFILE output */
typedef struct
{
	int uncompressed_size;
	int compressed_size;
	void *compressed_data;
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
	DATAFILE_OUT *df = (DATAFILE_OUT*)mem_alloc(sizeof(DATAFILE_OUT), 1);
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
	
	/*
	dbg_msg("datafile", "added item type=%d id=%d size=%d", type, id, size);
	int i;
	for(i = 0; i < size/4; i++)
		dbg_msg("datafile", "\t%d: %08x %d", i, ((int*)data)[i], ((int*)data)[i]);
	*/
	
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
	DATA_INFO *info = &df->datas[df->num_datas];
	unsigned long s = compressBound(size);
	void *compdata = mem_alloc(s, 1); /* temporary buffer that we use duing compression */

	int result = compress((Bytef*)compdata, &s, (Bytef*)data, size);
	if(result != Z_OK)
	{
		dbg_msg("datafile", "compression error %d", result);
		dbg_assert(0, "zlib error");
	}
		
	info->uncompressed_size = size;
	info->compressed_size = (int)s;
	info->compressed_data = mem_alloc(info->compressed_size, 1);
	mem_copy(info->compressed_data, compdata, info->compressed_size);
	mem_free(compdata);

	df->num_datas++;
	return df->num_datas-1;
}

int datafile_add_data_swapped(DATAFILE_OUT *df, int size, void *data)
{
#if defined(CONF_ARCH_ENDIAN_BIG)
	void *swapped = mem_alloc(size, 1); /* temporary buffer that we use duing compression */
	int index;
	mem_copy(swapped, data, size);
	swap_endian(&swapped, sizeof(int), size/sizeof(int));
	index = datafile_add_data(df, size, swapped);
	mem_free(swapped);
	return index;
#else
	return datafile_add_data(df, size, data);
#endif
}


int datafile_finish(DATAFILE_OUT *df)
{
	int itemsize = 0;
	int i, count, offset;
	int typessize, headersize, offsetsize, filesize, swapsize;
	int datasize = 0;
	DATAFILE_ITEM_TYPE info;
	DATAFILE_ITEM itm;
	DATAFILE_HEADER header;

	/* we should now write this file! */
	if(DEBUG)
		dbg_msg("datafile", "writing");

	/* calculate sizes */
	for(i = 0; i < df->num_items; i++)
	{
		if(DEBUG)
			dbg_msg("datafile", "item=%d size=%d (%d)", i, df->items[i].size, df->items[i].size+sizeof(DATAFILE_ITEM));
		itemsize += df->items[i].size + sizeof(DATAFILE_ITEM);
	}
	
	
	for(i = 0; i < df->num_datas; i++)
		datasize += df->datas[i].compressed_size;
	
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
	{
		header.id[0] = 'D';
		header.id[1] = 'A';
		header.id[2] = 'T';
		header.id[3] = 'A';
		header.version = 4;
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
	}
	
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
		offset += df->datas[i].compressed_size;
	}

	/* write data uncompressed sizes */
	for(i = 0, offset = 0; i < df->num_datas; i++)
	{
		/*
		if(DEBUG)
			dbg_msg("datafile", "writing data offset num=%d offset=%d", i, offset);
		*/
		io_write(df->file, &df->datas[i].uncompressed_size, sizeof(int));
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
			dbg_msg("datafile", "writing data id=%d size=%d", i, df->datas[i].compressed_size);
		io_write(df->file, df->datas[i].compressed_data, df->datas[i].compressed_size);
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

#define BUFFER_SIZE 64*1024

int datafile_crc(const char *filename)
{
	unsigned char buffer[BUFFER_SIZE];
	IOHANDLE file;
	int crc = 0;
	unsigned bytes = 0;
	
	file = io_open(filename, IOFLAG_READ);
	if(!file)
		return 0;
		
	while(1)
	{
		bytes = io_read(file, buffer, BUFFER_SIZE);
		if(bytes <= 0)
			break;
		crc = crc32(crc, buffer, bytes);
	}
	
	io_close(file);

	return crc;	
}
