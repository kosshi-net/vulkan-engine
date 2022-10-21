#include "mem.h"

#include "common.h"

#include <pthread.h>

/*
 * Memory allocating and debugging wrapper
 * 
 * The original motivation behind this allocator was to set a real memory
 * limit. I got tired of system malloc fragmenting things up so bad the programe
 * took like 2-3 times what it itself reported, leading my development machine
 * to lock up thanks to OOM.
 *
 * Of course, if your application actually has a leak, this will crash your 
 * program before your program crashes your computer.
 *
 * This is not intended to be fast. It's a simple and naive general allocator. 
 * Implementing a slab allocator may be something I'll do in the future - for
 * now this is purely for convenience. Performance-critical sections should 
 * not be messing with mallocs and frees anyway.
 *
 * Later on I expanded this into a proper fat pointer runtime. I was already
 * storing the size of every allocation - why not expose that to the 
 * application? 
 * 
 */

pthread_mutex_t mem_mtx = PTHREAD_MUTEX_INITIALIZER;

struct {
	void*  p;
	char* label;
	size_t size;
} AllocDebugInfo;

static struct {
	void  *heap;
	size_t size;
	size_t used;
	void (*error_callback)(enum MEM_ERROR);
} mem;

enum {
	MEM_FLAG_FREE = 1<<0
};

struct MemoryNode
{
	struct MemoryNode 	*prev;
	struct MemoryNode 	*next;
	uint32_t  			flags;

	size_t 	 			size;
	
	uint8_t             data[];
};

#define NODESIZE sizeof(struct MemoryNode)
#define ISFREE(node) (node->flags & MEM_FLAG_FREE)

struct MemoryNode *mem_tail;


static void print_node(struct MemoryNode *node) 
{
	printf(" node: %p\n", node);
	printf("  prev: %p\n", node->prev);
	printf("  next: %p\n", node->next);
	printf("  free; %i\n", node->flags);
	printf("  size: %liu\n", node->size);
	printf("  data: %p\n", node->data);

}
static void print_all_nodes(const char*title) {
	printf("%s\n", title);
	struct MemoryNode *node = mem.heap;
	do {
		print_node(node);
		if (node->next && node != node->next->prev)
			printf("ERROR: node != node->next->prev!\n");
		if (node->prev && node != node->prev->next)
			printf("ERROR: node != node->prev->next!\n");

	} while ( (node=node->next) );
}

void mem_debug(void) {
	struct MemoryNode *node = mem.heap;
	do {
		if (node->next && node != node->next->prev)
			printf("ERROR: node != node->next->prev!\n");
		if (node->prev && node != node->prev->next)
			printf("ERROR: node != node->prev->next!\n");
	} while ( (node=node->next) );
	printf("Test passed?\n");
}

//#define BEST_FIT

void *mem_block_alloc(size_t bytes)
{
	size_t align = NODESIZE-1;
	bytes = (bytes + align) & ~align;

	struct MemoryNode *best = NULL;
	struct MemoryNode *node = mem.heap;
#ifdef BEST_FIT	
	do { /* Find the smallest fitting block */
		if (ISFREE(node) && node->size >= bytes  
		 && (!best || node->size < best->size) )
			best = node;
	} while ((node=node->next));
#else
	do { /* Find first fitting block */
		if (ISFREE(node) && node->size >= bytes) {
			best = node;
			break;
		}
	} while ((node=node->next));
#endif
	if (best == NULL) {
		return NULL;
	}
	best->flags &= ~MEM_FLAG_FREE;
	/* if not enough room to split the node */
	if (best->size < bytes+NODESIZE+NODESIZE) return best+1;
	/* Create new node */
	size_t old_size = best->size;
	best->size = bytes;
	struct MemoryNode *new = best + (bytes / NODESIZE) + 1;
	new->flags = 0;
	new->prev = best;
	new->next = best->next;
	if (new->next) new->next->prev = new;
	new->size = old_size - best->size - NODESIZE;
	best->next = new;
	new->flags |= MEM_FLAG_FREE;
	return best + 1;
}

void mem_block_free(void *address)
{
	struct MemoryNode *node = &((struct MemoryNode*)address)[-1];
	node->flags |= MEM_FLAG_FREE;
	if (node->prev && ISFREE(node->prev)) 
		node = node->prev;	
	/* Merge all the next free nodes. */
	while (node->next && ISFREE(node->next)) { 
		node->size += node->next->size + NODESIZE;
		struct MemoryNode *del = node->next;
		node->next = del->next;
		if (node->next) node->next->prev = node;
	}
}

void mem_block_shrink(void *address, size_t bytes) 
{
	struct MemoryNode *node  = &((struct MemoryNode*)address)[-1];
	struct MemoryNode *new   = node + (bytes / NODESIZE) + 1;
	struct MemoryNode *merge = node->next;

	new->flags = 0 | MEM_FLAG_FREE;

	size_t move = node->size - bytes;
	node->size = bytes;
	new->size  = move - NODESIZE;
	node->next = new;
	new->prev  = node;
	new->next  = merge;
	if (merge == NULL) return;
	merge->prev = new;
	if (ISFREE(merge)) { 
		new->size += merge->size + NODESIZE;
		new->next = merge->next;
		if (new->next) new->next->prev = new;
	}

}

bool mem_block_expand(void *address, size_t bytes)
{
	struct MemoryNode *node = &((struct MemoryNode*)address)[-1];
	struct MemoryNode *merge = node->next;
	
	size_t required = bytes - node->size;

	if (merge && ISFREE(merge) && merge->size >= required) {

		/* Merge */
		node->size += merge->size + NODESIZE;
		node->next  = merge->next;
		if (node->next) node->next->prev = node;

		return true;
	} else {
		return false;
	}
}

int mem_init(size_t heap_size)
{
	mem.size = heap_size;
	mem.heap = malloc( mem.size );

	if (mem.heap == NULL) return 1;

	struct MemoryNode *head = mem.heap;
	head->prev = NULL;
	head->next = NULL;
	head->size = heap_size - NODESIZE - NODESIZE;
	head->flags |= MEM_FLAG_FREE;

	struct MemoryNode *tail = (void*)((uint8_t*)mem.heap + mem.size - NODESIZE);
	tail->prev = head;
	tail->next = NULL;
	tail->size = 0;
	tail->flags |= MEM_FLAG_FREE;

	mem_tail = tail;

	head->next = tail;
	return 0;
}

size_t mem_used(void)
{
	return mem.used;
}

void *mem_alloc(size_t bytes)
{
	pthread_mutex_lock( &mem_mtx );

	void* p = mem_block_alloc(bytes);

	if (p == NULL) {
		if(mem.error_callback) mem.error_callback(MEM_ERROR_UNKNOWN);
		return NULL;
	}
	mem.used += bytes;
	pthread_mutex_unlock( &mem_mtx );
	return p; 
}

void *mem_calloc(size_t bytes)
{
	pthread_mutex_lock( &mem_mtx );

	void* p = mem_block_alloc(bytes);
	if (p == NULL) {
		if(mem.error_callback) mem.error_callback(MEM_ERROR_UNKNOWN);
		return NULL;
	}

	mem.used += bytes;

	memset(p, 0, bytes);
	pthread_mutex_unlock( &mem_mtx );

	return p;
}

void *mem_resize(void *p, size_t new_size)
{
	pthread_mutex_lock( &mem_mtx );

	size_t orig_size = mem_size(p);
	mem.used -= orig_size;

	size_t align = NODESIZE-1;
	orig_size = (orig_size + align) & ~align;
	new_size  =  (new_size + align) & ~align;

	if (orig_size == new_size) {
		pthread_mutex_unlock( &mem_mtx );
		return p;
	}
	
	if (new_size < orig_size) {
		mem_block_shrink(p, new_size);
		pthread_mutex_unlock( &mem_mtx );
		return p;
	}

	if(1)
	if (mem_block_expand(p, new_size)) {
		mem_block_shrink(p, new_size);
		mem.used += mem_size(p);
		pthread_mutex_unlock( &mem_mtx );
		return p;
	}
	void *new = mem_block_alloc(new_size);

	if (new == NULL) {
		if(mem.error_callback) mem.error_callback(MEM_ERROR_UNKNOWN);
		return NULL;
	}
	
	memcpy(new, p, new_size);

	mem.used += mem_size(new);

	mem_block_free(p);

	pthread_mutex_unlock( &mem_mtx );

	return new;
}

void *mem_free(void *p)
{
	pthread_mutex_lock( &mem_mtx );


	if (p == NULL) {

		pthread_mutex_unlock( &mem_mtx );
		return NULL;
	}

	mem.used -= mem_size(p);
		
	mem_block_free(p);
	pthread_mutex_unlock( &mem_mtx );

	return NULL;
}

void mem_set_error_callback( void(*fn)(enum MEM_ERROR) )
{
	mem.error_callback = fn;
}

size_t mem_size(void *address)
{
	struct MemoryNode *node = &((struct MemoryNode*)address)[-1];
	return node->size;
}


