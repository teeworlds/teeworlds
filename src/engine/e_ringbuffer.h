#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

typedef struct RINGBUFFER RINGBUFFER;

enum
{
	/* Will start to destroy items to try to fit the next one */
	RINGBUF_FLAG_RECYCLE=1
};
 
RINGBUFFER *ringbuf_init(void *memory, int size, int flags);
void ringbuf_clear(RINGBUFFER *rb);
void *ringbuf_allocate(RINGBUFFER *rb, int size);
void ringbuf_validate(RINGBUFFER *rb);

void *ringbuf_item_ptr(void *p);

void *ringbuf_prev(RINGBUFFER *rb, void *current);
void *ringbuf_next(RINGBUFFER *rb, void *current);
void *ringbuf_first(RINGBUFFER *rb);
void *ringbuf_last(RINGBUFFER *rb);

void ringbuf_popfirst(RINGBUFFER *rb);

#endif
