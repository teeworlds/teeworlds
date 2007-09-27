
typedef struct HEAP_t HEAP;
HEAP *memheap_create();
void memheap_destroy(HEAP *heap);
void *memheap_allocate(HEAP *heap, int size);
