#ifndef FILE_SNAPSHOT_H
#define FILE_SNAPSHOT_H

struct snapshot
{
	int data_size;
	int num_items;
	
	struct item
	{
		int type_and_id;

		int *data() { return (int *)(this+1); }		
		int type() { return type_and_id>>16; }
		int id() { return type_and_id&(0xffff); }
		int key() { return type_and_id; }
	};

	int *offsets() { return (int *)(this+1); }		
	char *data_start() { return (char *)(offsets() + num_items); }
	item *get_item(int index) { return (item *)(data_start() + offsets()[index]); };
	
	// returns the number of ints in the item data
	int get_item_datasize(int index)
	{
		if(index == num_items-1)
			return (data_size - offsets()[index]) - sizeof(item);
		return (offsets()[index+1] - offsets()[index]) - sizeof(item);
	}
	
	int get_item_index(int key)
	{
		// TODO: this should not be a linear search. very bad
		for(int i = 0; i < num_items; i++)
		{
			if(get_item(i)->key() == key)
				return i;
		}
		return -1;
	}
};

void *snapshot_empty_delta();
int snapshot_crc(snapshot *snap);
void snapshot_debug_dump(snapshot *snap);
int snapshot_create_delta(snapshot *from, snapshot *to, void *data);
int snapshot_unpack_delta(snapshot *from, snapshot *to, void *data, int data_size);

#endif // FILE_SNAPSHOT_H
