
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
 
RINGBUFFER *rb_init(void *memory, int size);
void *rb_allocate(RINGBUFFER *rb, int size);
void rb_validate(RINGBUFFER *rb);

void *rb_item_ptr(void *p);

void *rb_prev(RINGBUFFER *rb, void *current);
void *rb_next(RINGBUFFER *rb, void *current);
void *rb_first(RINGBUFFER *rb);
void *rb_last(RINGBUFFER *rb);
