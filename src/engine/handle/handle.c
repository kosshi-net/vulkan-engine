#include "handle/handle.h"

static uint32_t type_counter = 1;

void handle_allocator_create(struct HandleAllocator *this)
{
	size_t head_size   = sizeof(uint32_t) * this->item_max;
	size_t buffer_size = this->item_size  * this->item_max;

	this->handle = calloc( head_size + buffer_size, 1 );
	this->buffer = ((uint8_t*)this->handle)+head_size;

	if (!this->type_hash)
		this->type_hash = type_counter++;
}

Handle handle_alloc(struct HandleAllocator *restrict this)
{
	if (this->buffer == NULL) 
		handle_allocator_create(this);
	
	for (uint32_t i = 0; i < this->item_max; i++) {
		if (this->handle[i]) 
			continue;
		
		this->handle[i]  = i+1;
		this->handle[i] |= this->item_hash << 16;
		this->handle[i] |= this->type_hash << 24;

		this->item_hash++;

		return this->handle[i];
	}
	
	engine_crash("Out of items");
	return 0;
}

static inline uint32_t handle_index(
	struct HandleAllocator *restrict this, 
	Handle handle)
{
	uint32_t index = (handle & 0xFFFF) - 1;
	uint32_t type  = (handle >> 24);

	if (handle == 0)
		engine_crash("NULL handle");

	if (this->handle[index] != handle) {
		if (type != this->type_hash)
			engine_crash("Wrong handle type");
		engine_crash("Stale handle");
	}

	return index;
}

void handle_free(struct HandleAllocator *restrict this, Handle *handle)
{
	uint32_t index = handle_index(this, *handle);
	this->handle[index] = 0;
	*handle = 0;
}

void *handle_deref(struct HandleAllocator *this, Handle handle)
{
	uint32_t index = handle_index(this, handle);
	return this->buffer + index * this->item_size;
}

