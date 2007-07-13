
// TODO: remove all the allocations from this class
class ring_buffer
{
public:
	struct item
	{
		item *next;
		item *prev;
		int size;
		unsigned char *data() { return (unsigned char *)(this+1); }
	};
	
	item *first_item;
	item *last_item;
	
	unsigned buffer_size;
	
	ring_buffer()
	{
		first_item = 0;
		last_item = 0;
		buffer_size = 0;
	}
	
	~ring_buffer()
	{
		reset();
	}
	
	void reset()
	{
		// clear all
		while(first())
			pop_first();
	}
	
	void *alloc(int size)
	{
		item *i = (item*)mem_alloc(sizeof(item)+size, 1);
		i->size = size;
		
		i->prev = last_item;
		i->next = 0;
		if(last_item)
			last_item->next = i;
		else
			first_item = i;
		last_item = i;
		
		buffer_size += size;
		return i->data();
	}
	
	item *first()
	{
		return first_item;
	}

	/*
	void *peek_data()
	{
		if(!first)
			return 0;
		return (void*)(first+1);
	}*/
	
	void pop_first()
	{
		if(first_item)
		{
			item *next = first_item->next;
			buffer_size -= first_item->size;
			mem_free(first_item);
			first_item = next;
			if(first_item)
				first_item->prev = 0;
			else
				last_item = 0;
		}
	}
	
	unsigned size() { return buffer_size; }
};
