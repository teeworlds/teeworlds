/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "e_system.h"

typedef struct CHUNK_t
{
	char *memory;
	char *current;
	char *end;
	struct CHUNK_t *next;
} CHUNK;

typedef struct 
{
	CHUNK *current;
} HEAP;

/* how large each chunk should be */
static const int chunksize = 1024*64;

/* allocates a new chunk to be used */
static CHUNK *memheap_newchunk()
{
	CHUNK *chunk;
	char *mem;
	
	/* allocate memory */
	mem = (char*)mem_alloc(sizeof(CHUNK)+chunksize, 1);
	if(!mem)
		return 0x0;

	/* the chunk structure is located in the begining of the chunk */
	/* init it and return the chunk */
	chunk = (CHUNK*)mem;
	chunk->memory = (char*)(chunk+1);
	chunk->current = chunk->memory;
	chunk->end = chunk->memory + chunksize;
	chunk->next = (CHUNK *)0x0;
	return chunk;
}

/******************/
static void *memheap_allocate_from_chunk(CHUNK *chunk, unsigned int size)
{
	char *mem;
	
	/* check if we need can fit the allocation */
	if(chunk->current + size > chunk->end)
		return (void*)0x0;

	/* get memory and move the pointer forward */
	mem = chunk->current;
	chunk->current += size;
	return mem;
}

/* creates a heap */
HEAP *memheap_create()
{
	CHUNK *chunk;
	HEAP *heap;
	
	/* allocate a chunk and allocate the heap structure on that chunk */
	chunk = memheap_newchunk();
	heap = (HEAP *)memheap_allocate_from_chunk(chunk, sizeof(HEAP));
	heap->current = chunk;
	return heap;
}

/* destroys the heap */
void memheap_destroy(HEAP *heap)
{
	CHUNK *chunk = heap->current;
	CHUNK *next;
	
	while(chunk)
	{
		next = chunk->next;
		mem_free(chunk);
		chunk = next;
	}
}

/* */
void *memheap_allocate(HEAP *heap, unsigned int size)
{
	char *mem;

	/* try to allocate from current chunk */
	mem = (char *)memheap_allocate_from_chunk(heap->current, size);
	if(!mem)
	{
		/* allocate new chunk and add it to the heap */
		CHUNK *chunk = memheap_newchunk();
		chunk->next = heap->current;
		heap->current = chunk;
		
		/* try to allocate again */
		mem = (char *)memheap_allocate_from_chunk(heap->current, size);
	}
	
	return mem;
}
