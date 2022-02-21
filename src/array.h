#ifndef _ARRAY_H
#define _ARRAY_H

#include <stddef.h>

void*array_create(size_t item_size);

/* Add item and possibly reallocate. 
 * Pass by address. */
void array_push(void*, const void*data);

/* Pass by address */
size_t array_length(void*);

/* Returns length * sizeof element
 * Pass by address */
size_t array_sizeof(void*);

/* Pass by address */
void array_destroy(void*);

/* Allocate and add a number of elements, but do not write anything. Pointer to
 * allocated block is returned. 
 * NOTE: Return pointer becomes invalid if any reallocating array functions are 
 * called. 
 * Pass by address. */
void*array_reserve(void*, size_t count);

#endif
