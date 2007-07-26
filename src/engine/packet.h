#include <stdarg.h>
#include <baselib/stream/file.h>
#include <baselib/network.h>

#include "versions.h"
#include "ringbuffer.h"
#include "compression.h"
#include "snapshot.h"

enum
{
	NETMSG_NULL=0,
	
	// sent by server
	NETMSG_MAP,
	NETMSG_SNAP,
	NETMSG_SNAPEMPTY,
	NETMSG_SNAPSMALL,
	
	// sent by client
	NETMSG_INFO,
	NETMSG_ENTERGAME,
	NETMSG_INPUT,
	NETMSG_SNAPACK,
	
	// sent by both
	NETMSG_ERROR,
};


// this should be revised
enum
{
	MAX_NAME_LENGTH=32,
	MAX_CLANNAME_LENGTH=32,
	MAX_INPUT_SIZE=128,
	MAX_SNAPSHOT_SIZE=64*1024,
	MAX_SNAPSHOT_PACKSIZE=768
};


class snapshot_storage
{
	struct holder
	{
		int64 tagtime;
		int tick;
		int data_size;
		int *data() { return (int *)(this+1); }
	};
	
	ring_buffer buffer;
	
public:
	void purge_until(int tick)
	{
		while(1)
		{
			ring_buffer::item *i = buffer.first();
			if(!i)
				break;
			holder *h = (holder *)i->data();
			if(h->tick < tick)
				buffer.pop_first();
			else
				break;
		}
	}
	
	void purge_all()
	{
		buffer.reset();
	}

	void add(int tick, int64 tagtime, int data_size, void *data)
	{
		holder *h = (holder *)buffer.alloc(sizeof(holder)+data_size);
		h->tick = tick;
		h->data_size = data_size;
		h->tagtime = tagtime;
		mem_copy(h->data(), data, data_size);
	}
	
	int get(int tick, int64 *tagtime, void **data)
	{
		ring_buffer::item *i = buffer.first();
		while(i)
		{
			holder *h = (holder *)i->data();
			if(h->tick == tick)
			{
				if(data)
					*data = h->data();
				if(tagtime)
					*tagtime = h->tagtime;
				return h->data_size;
			}
				
			i = i->next;
		}
		
		return -1;
	}
};

class snapshot_builder
{
public:
	static const int MAX_ITEMS = 512;

	char data[MAX_SNAPSHOT_SIZE];
	int data_size;

	int offsets[MAX_ITEMS];
	int num_items;

	int top_size;
	int top_items;

	int snapnum;

	snapshot_builder()
	{
		top_size = 0;
		top_items = 0;
		snapnum = 0;
	}

	void start()
	{
		data_size = 0;
		num_items = 0;
	}
	
	snapshot::item *get_item(int index) 
	{
		return (snapshot::item *)&(data[offsets[index]]);
	}
	
	int *get_item_data(int key)
	{
		for(int i = 0; i < num_items; i++)
		{
			if(get_item(i)->key() == key)
				return (int *)get_item(i)->data();
		}
		return 0;
	}

	int finish(void *snapdata)
	{
		snapnum++;

		// flattern and make the snapshot
		snapshot *snap = (snapshot *)snapdata;
		snap->data_size = data_size;
		snap->num_items = num_items;
		int offset_size = sizeof(int)*num_items;
		mem_copy(snap->offsets(), offsets, offset_size);
		mem_copy(snap->data_start(), data, data_size);
		return sizeof(snapshot) + offset_size + data_size;
	}

	void *new_item(int type, int id, int size)
	{
		snapshot::item *obj = (snapshot::item *)(data+data_size);
		obj->type_and_id = (type<<16)|id;
		offsets[num_items] = data_size;
		data_size += sizeof(snapshot::item) + size;
		num_items++;
		dbg_assert(data_size < MAX_SNAPSHOT_SIZE, "too much data");
		dbg_assert(num_items < MAX_ITEMS, "too many items");

		return obj->data();
	}
};

class data_packer
{
	enum
	{
		BUFFER_SIZE=1024*2
	};
	
	unsigned char buffer[BUFFER_SIZE];
	unsigned char *current;
	unsigned char *end;
	int error;
public:
	void reset()
	{
		error = 0;
		current = buffer;
		end = current + BUFFER_SIZE;
	}
	
	void add_int(int i)
	{
		// TODO: add space check
		// TODO: variable length encoding perhaps
		// TODO: add debug marker
		current = vint_pack(current, i);
		//*current++ = (i>>24)&0xff;
		//*current++ = (i>>16)&0xff;
		//*current++ = (i>>8)&0xff;
		//*current++ = i&0xff;
	}

	void add_string(const char *p, int limit)
	{
		// TODO: add space check
		// TODO: add debug marker
		if(limit > 0)
		{
			while(*p && limit != 0)
			{
				*current++ = *p++;
				limit--;
			}
			*current++ = 0;
		}
		else
		{
			while(*p)
				*current++ = *p++;
			*current++ = 0;
		}
	}
	
	void add_raw(const unsigned char *data, int size)
	{
		// TODO: add space check
		// TODO: add debug marker
		//add_int(size);
		while(size)
		{
			*current++ = *data++;
			size--;
		}
	}
	
	int size() const
	{
		return (const unsigned char *)current-(const unsigned char *)buffer;
	}
	
	const unsigned char *data()
	{
		return (const unsigned char *)buffer;
	}
};

class data_unpacker
{
	const unsigned char *current;
	const unsigned char *start;
	const unsigned char *end;
	int error;
	
public:
	void reset(const unsigned char *data, int size)
	{
		error = 0;
		start = data;
		end = start + size;
		current = start;
	}
	
	int get_int()
	{
		int i;
		current = vint_unpack(current, &i);
		// TODO: might be changed into variable width
		// TODO: add range check
		// TODO: add debug marker
		//i = (current[0]<<24) | (current[1]<<16) | (current[2]<<8) | (current[3]);
		//current += 4;
		return i;
	}
	
	const char *get_string()
	{
		// TODO: add range check
		// TODO: add debug marker
		const char *ptr = (const char *)current;
		while(*current) // skip the string
			current++;
		current++;
		return ptr;
	}
	
	const unsigned char *get_raw(int size)
	{
		// TODO: add range check
		// TODO: add debug marker
		//int s = get_int();
		//if(size)
			//*size = s;
		const unsigned char *ptr = current;
		current += size;
		return ptr;
	}
};
