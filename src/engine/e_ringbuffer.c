#include <base/system.h>

#include "e_ringbuffer.h"

typedef struct RBITEM
{
    struct RBITEM *prev;
    struct RBITEM *next;
    int free;
    int size;
} RBITEM;

/*

*/
struct RINGBUFFER
{
	RBITEM *produce;
	RBITEM *consume;
	
	RBITEM *first;
	RBITEM *last;
	void *memory;
	int size;
	int flags;
};

RINGBUFFER *ringbuf_init(void *memory, int size, int flags)
{
	RINGBUFFER *rb = (RINGBUFFER *)memory;
	mem_zero(memory, size);
	
	rb->memory = rb+1;
	rb->size = (size-sizeof(RINGBUFFER))/sizeof(RBITEM)*sizeof(RBITEM);
	rb->first = (RBITEM *)rb->memory;
	rb->first->free = 1;
	rb->first->size = rb->size;
	rb->last = rb->first;
	rb->produce = rb->first;
	rb->consume = rb->first;
	
	rb->flags = flags;
	
	return rb;
}

static RBITEM *ringbuf_nextblock(RINGBUFFER *rb, RBITEM *item)
{
	if(item->next)
		return item->next;
	return rb->first;
}

static RBITEM *ringbuf_prevblock(RINGBUFFER *rb, RBITEM *item)
{
	if(item->prev)
		return item->prev;
	return rb->last;
}

static RBITEM *ringbuf_mergeback(RINGBUFFER *rb, RBITEM *item)
{
	/* make sure that this block and previous block is free */
	if(!item->free || !item->prev || !item->prev->free)
		return item;

	/* merge the blocks */
	item->prev->size += item->size;
	item->prev->next = item->next;
	
	/* fixup pointers */
	if(item->next)
		item->next->prev = item->prev;
	else
		rb->last = item->prev;
		
	if(item == rb->produce)
		rb->produce = item->prev;
	
	if(item == rb->consume)
		rb->consume = item->prev;
	
	/* return the current block */
	return item->prev;
}

int ringbuf_popfirst(RINGBUFFER *rb)
{
	if(rb->consume->free)
		return 0;
	
	/* set the free flag */
	rb->consume->free = 1;
	
	/* previous block is also free, merge them */
	rb->consume = ringbuf_mergeback(rb, rb->consume);
	
	/* advance the consume pointer */
	rb->consume = ringbuf_nextblock(rb, rb->consume);
	while(rb->consume->free && rb->consume != rb->produce)
	{
		rb->consume = ringbuf_mergeback(rb, rb->consume);
		rb->consume = ringbuf_nextblock(rb, rb->consume);
	}
		
	/* in the case that we have catched up with the produce pointer */
	/* we might stand on a free block so merge em */
	ringbuf_mergeback(rb, rb->consume);
	return 1;
}

void *ringbuf_allocate(RINGBUFFER *rb, int size)
{
	int wanted_size = (size+sizeof(RBITEM)+sizeof(RBITEM)-1)/sizeof(RBITEM)*sizeof(RBITEM);
	RBITEM *block = 0;

	/* check if we even can fit this block */
	if(wanted_size > rb->size)
		return 0;

	while(1)	
	{
		/* check for space */
		if(rb->produce->free)
		{
			if(rb->produce->size >= wanted_size)
				block = rb->produce;
			else
			{
				/* wrap around to try to find a block */
				if(rb->first->free && rb->first->size >= wanted_size)
					block = rb->first;
			}
		}
		
		if(block)
			break;
		else
		{
			/* we have no block, check our policy and see what todo */
			if(rb->flags&RINGBUF_FLAG_RECYCLE)
			{
				if(!ringbuf_popfirst(rb))
					return 0;
			}
			else
				return 0;
		}
	}
	
	/* okey, we have our block */
	
	/* split the block if needed */
	if(block->size > wanted_size)
	{
		RBITEM *new_item = (RBITEM *)((char *)block + wanted_size);
		new_item->prev = block;
		new_item->next = block->next;
		if(new_item->next)
			new_item->next->prev = new_item;
		block->next = new_item;
		
		new_item->free = 1;
		new_item->size = block->size - wanted_size;
		block->size = wanted_size;
		
		if(!new_item->next)
			rb->last = new_item;
	}
	
	
	/* set next block */
	rb->produce = ringbuf_nextblock(rb, block);
	
	/* set as used and return the item pointer */
	block->free = 0;
	return block+1;
}

void *ringbuf_prev(RINGBUFFER *rb, void *current)
{
	RBITEM *item = ((RBITEM *)current) - 1;
	
	while(1)
	{
		item = ringbuf_prevblock(rb, item);
		if(item == rb->produce)
			return 0;
		if(!item->free)
			return item+1;
	}
}

void *ringbuf_next(RINGBUFFER *rb, void *current)
{
	RBITEM *item = ((RBITEM *)current) - 1;
	
	while(1)
	{
		item = ringbuf_nextblock(rb, item);
		if(item == rb->produce)
			return 0;
		if(!item->free)
			return item+1;
	}
}

void *ringbuf_first(RINGBUFFER *rb)
{
	if(rb->consume->free)
		return 0;
	return rb->consume+1;
}

void *ringbuf_last(RINGBUFFER *rb)
{
	if(!rb->produce->free)
		return rb->produce+1;
	return ringbuf_prev(rb, rb->produce+1);
}


/* debugging and testing stuff */

static void ringbuf_debugdump(RINGBUFFER *rb, const char *msg)
{
	RBITEM *cur = rb->first;
	
	dbg_msg("ringbuf", "-- dumping --");
	
	while(cur)
	{
		char flags[4] = "   ";
		if(cur->free)
			flags[0] = 'F';
		if(cur == rb->consume)
			flags[1] = '>';
		if(cur == rb->produce)
			flags[2] = '<';
		dbg_msg("ringbuf", "%s %d", flags, cur->size);
		cur = cur->next;
	}
	
	dbg_msg("ringbuf", "--  --");
	
	if(msg)
		dbg_assert(0, msg);
}



static void ringbuf_validate(RINGBUFFER *rb)
{
	RBITEM *prev = 0;
	RBITEM *cur = rb->first;
	int freechunks = 0;
	int got_consume = 0;
	int got_produce = 0;
	
	while(cur)
	{
		
		if(cur->free)
			freechunks++;
		
		if(freechunks > 2) ringbuf_debugdump(rb, "too many free chunks");
		if(prev && prev->free && cur->free) ringbuf_debugdump(rb, "two free chunks next to each other");
		if(cur == rb->consume) got_consume = 1;
		if(cur == rb->produce) got_produce = 1;

		dbg_assert(cur->prev == prev, "prev pointers doesn't match");
		dbg_assert(!prev || prev->next == cur, "next pointers doesn't match");
		dbg_assert(cur->next || cur == rb->last, "last isn't last");

		prev = cur;
		cur = cur->next;
	}

	if(!got_consume) ringbuf_debugdump(rb, "consume pointer isn't pointing at a valid block");
	if(!got_produce) ringbuf_debugdump(rb, "produce pointer isn't pointing at a valid block");
}

int ringbuf_test()
{
	char buffer[256];
	RINGBUFFER *rb;
	int i, s, k, m;
	int count;
	int testcount = 0;
	
	void *item;
	int before;
	
	
	for(k = 100; k < sizeof(buffer); k++)
	{
		if((k%10) == 0)
			dbg_msg("ringbuf", "testing at %d", k);
		rb = ringbuf_init(buffer, k, 0);
		count = 0;
		
		for(s = 1; s < sizeof(buffer); s++)
		{
			for(i = 0; i < k*8; i++, testcount++)
			{
				for(m = 0, item = ringbuf_first(rb); item; item = ringbuf_next(rb, item), m++);
				before = m;
				
				if(ringbuf_allocate(rb, s))
				{
					count++;
					for(m = 0, item = ringbuf_first(rb); item; item = ringbuf_next(rb, item), m++);
					if(before+1 != m) ringbuf_debugdump(rb, "alloc error");
					if(count != m) ringbuf_debugdump(rb, "count error");
				}
				else
				{
					if(ringbuf_popfirst(rb))
					{
						count--;
						
						for(m = 0, item = ringbuf_first(rb); item; item = ringbuf_next(rb, item), m++);
						if(before-1 != m) dbg_msg("", "popping error %d %d", before, m);
						if(count != m) ringbuf_debugdump(rb, "count error");
					}
				}
				
				/* remove an item every 10 */
				if((i%10) == 0)
				{
					for(m = 0, item = ringbuf_first(rb); item; item = ringbuf_next(rb, item), m++);
					before = m;
					
					if(ringbuf_popfirst(rb))
					{
						count--;
						for(m = 0, item = ringbuf_first(rb); item; item = ringbuf_next(rb, item), m++);
						if(before-1 != m) dbg_msg("", "popping error %d %d", before, m);
						dbg_assert(count == m, "count error");
					}
				}
				
				/* count items */
				for(m = 0, item = ringbuf_first(rb); item; item = ringbuf_next(rb, item), m++);
				if(m != count) ringbuf_debugdump(rb, "wrong number of items during forward count");

				for(m = 0, item = ringbuf_last(rb); item; item = ringbuf_prev(rb, item), m++);
				if(m != count) ringbuf_debugdump(rb, "wrong number of items during backward count");
					
				ringbuf_validate(rb);
			}

			/* empty the ring buffer */			
			while(ringbuf_first(rb))
				ringbuf_popfirst(rb);
			ringbuf_validate(rb);
			count = 0;
		}
	}
	
	return 0;
}
