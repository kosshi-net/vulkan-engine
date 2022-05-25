#pragma once 
#include "common.h"

/* 
 * Any sufficiently complicated C or Fortran program contains an ad hoc, 
 * informally-specified, bug-ridden, slow implementation of an slab allocator.
 */

/*
 * Handles only support 65535 objects as half of the bits of the 32bit handles
 * are used for runtime checks.
 */

#define HANDLE_ALLOCATOR(T, num) { .item_size = sizeof(T), .item_max = num}

struct HandleAllocator {
	uint32_t item_size;
	uint32_t item_max;
	uint32_t item_count;

	uint8_t  item_hash;
	uint8_t  type_hash;

	Handle   *handle;
	uint8_t  *buffer;
};


Handle handle_allocate   (struct HandleAllocator *);
void   handle_free       (struct HandleAllocator *, Handle *);
void  *handle_dereference(struct HandleAllocator *, Handle handle);

