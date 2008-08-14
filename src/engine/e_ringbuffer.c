#include <base/system.h>

enum
{
	RBFLAG_FREE=1
};

typedef struct RBITEM_t
{
    struct RBITEM_t *prev;
    struct RBITEM_t *next;
    int flags;
    int size;
} RBITEM;

typedef struct
{
    /* what you need */
    RBITEM *next_alloc;
    RBITEM *last_alloc;
    RBITEM *first;
    RBITEM *last;
    void *memory;
    int size;
} RINGBUFFER; 
 
RINGBUFFER *ringbuf_init(void *memory, int size)
{
	RINGBUFFER *rb = (RINGBUFFER *)memory;
	mem_zero(memory, size);
	
	rb->memory = rb+1;
	rb->size = (size-sizeof(RINGBUFFER))/sizeof(RBITEM)*sizeof(RBITEM);
	rb->first = (RBITEM *)rb->memory;
	rb->first->flags = RBFLAG_FREE;
	rb->first->size = rb->size;
	rb->last = rb->first;
	rb->next_alloc = rb->first;
	
	return rb;
}

static RBITEM *ringbuf_free(RINGBUFFER *rb, RBITEM *item)
{
	dbg_assert(!(item->flags&RBFLAG_FREE), "trying to  free element that is already freed");
	item->flags |= RBFLAG_FREE;

	/* merge with all free items backwards */
	while(item->prev && (item->prev->flags&RBFLAG_FREE))
	{
		item->prev->size += item->size;
		item->prev->next = item->next;
		item = item->prev;
		if(item->next)
			item->next->prev = item;
	}

	/* merge with all free items forwards */
	while(item->next && (item->next->flags&RBFLAG_FREE))
	{
		item->size += item->next->size;
		item->next = item->next->next;
		if(item->next)
			item->next->prev = item;
	}
	
	if(!item->next)
		rb->last = item;
	
	return item;
}

void ringbuf_validate(RINGBUFFER *rb)
{
	RBITEM *prev = 0;
	RBITEM *cur = rb->first;
	int freechunks = 0;
	
	while(cur)
	{
		if(cur->flags&RBFLAG_FREE)
			freechunks++;
			
		dbg_assert(freechunks <= 2, "too many free chunks");
		dbg_assert(cur->prev == prev, "pointers doesn't match");
		dbg_assert(!prev || prev->next == cur, "pointers doesn't match");
		dbg_assert(cur->next || cur == rb->last, "last isn't last");

		prev = cur;
		cur = cur->next;
	}
}

static RBITEM *ringbuf_try_allocate(RINGBUFFER *rb, int wanted_size)
{
	RBITEM *item = rb->next_alloc;
	
	/* check for space */
	if(!(item->flags&RBFLAG_FREE) || item->size < wanted_size)
		return 0x0;
		
	/* split the block if needed */
	if(item->size > wanted_size)
	{
		RBITEM *new_item = (RBITEM *)((char *)item + wanted_size);
		new_item->prev = item;
		new_item->next = item->next;
		if(new_item->next)
			new_item->next->prev = new_item;
		item->next = new_item;
		
		new_item->flags = RBFLAG_FREE;
		new_item->size = item->size - wanted_size;
		item->size = wanted_size;
		
		if(!new_item->next)
			rb->last = new_item;
	}
	
	item->flags &= ~RBFLAG_FREE;
	rb->next_alloc = item->next;
	if(!rb->next_alloc)
		rb->next_alloc = rb->first;
		
	rb->last_alloc = item;
	return item;
}

void *ringbuf_allocate(RINGBUFFER *rb, int size)
{
	int wanted_size = (size+sizeof(RBITEM)+sizeof(RBITEM)-1)/sizeof(RBITEM)*sizeof(RBITEM);
	RBITEM *block = 0;

	/* check if we even can fit this block */
	if(wanted_size > rb->size)
		return 0;

	/* free the block if it is in use */
	if(!(rb->next_alloc->flags&RBFLAG_FREE))
		rb->next_alloc = ringbuf_free(rb, rb->next_alloc);
		
	/* first attempt */
	block = ringbuf_try_allocate(rb, wanted_size);
	if(block)
		return block+1;
	
	/* ok, we need to wipe some blocks in order to get space */
	while(1)
	{
		if(rb->next_alloc->next)
			rb->next_alloc = ringbuf_free(rb, rb->next_alloc->next);
		else
		{
			rb->next_alloc = rb->first;
			rb->next_alloc = ringbuf_free(rb, rb->next_alloc);
		}

		/* try allocate again */
		block = ringbuf_try_allocate(rb, wanted_size);
		if(block)
			return block+1;
	}
}

void *ringbuf_prev(RINGBUFFER *rb, void *current)
{
	RBITEM *item = ((RBITEM *)current) - 1;
	while(1)
	{
		/* back up one step */
		item = item->prev;
		if(!item)
			item = rb->last;
			
		/* we have gone around */
		if(item == rb->last_alloc)
			return 0;
			
		if(!(item->flags&RBFLAG_FREE))
			return item+1;
	}
}


void *ringbuf_next(RINGBUFFER *rb, void *current)
{
	RBITEM *item = ((RBITEM *)current) - 1;

	/* we have gone around */
	if(item == rb->last_alloc)
		return 0;

	while(1)
	{
		/* back up one step */
		item = item->next;
		if(!item)
			item = rb->first;
			
		if(!(item->flags&RBFLAG_FREE))
			return item+1;
	}
}

void *ringbuf_item_ptr(void *p)
{
	return ((RBITEM *)p) - 1;
}

void *ringbuf_first(RINGBUFFER *rb)
{
	if(rb->last_alloc && rb->last_alloc->next)
		return ringbuf_next(rb, rb->last_alloc->next+1);
	return 0x0;
}

void *ringbuf_last(RINGBUFFER *rb)
{
	if(rb->last_alloc == 0)
		return 0;
	return rb->last_alloc+1;
}
