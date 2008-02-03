
typedef struct
{
	/* what you need */
	struct RBITEM_t *next_alloc;
	struct RBITEM_t *last_alloc;
	struct RBITEM_t *first;
	struct RBITEM_t *last;
	void *memory;
	int size;
} RINGBUFFER; 
 
RINGBUFFER *ringbuf_init(void *memory, int size);
void *ringbuf_allocate(RINGBUFFER *rb, int size);
void ringbuf_validate(RINGBUFFER *rb);

void *ringbuf_item_ptr(void *p);

void *ringbuf_prev(RINGBUFFER *rb, void *current);
void *ringbuf_next(RINGBUFFER *rb, void *current);
void *ringbuf_first(RINGBUFFER *rb);
void *ringbuf_last(RINGBUFFER *rb);
