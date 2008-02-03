
typedef struct RINGBUFFER;

typedef struct
{
	/* what you need */
	struct RBITEM *next_alloc;
	struct RBITEM *last_alloc;
	struct RBITEM *first;
	struct RBITEM *last;
	void *memory;
	int size;
} RINGBUFFER; 
 
RINGBUFFER *rb_init(void *memory, int size;
void *rb_allocate(RINGBUFFER *rb, int size);

void *rb_prev(RINGBUFFER *rb, void *current);
void *rb_next(RINGBUFFER *rb, void *current);
void *rb_first(RINGBUFFER *rb);
void *rb_last(RINGBUFFER *rb);
