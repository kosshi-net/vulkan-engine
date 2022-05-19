#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <stdio.h>

#ifndef ARR_malloc
#define ARR_malloc malloc 
#endif

#ifndef ARR_realloc
#define ARR_realloc realloc 
#endif

#ifndef ARR_free
#define ARR_free free 
#endif

#define Array(T) T *
#define ARR_get_header(p) &(*(struct ARR_Head**)p)[-1]

struct ARR_Head {
	size_t  length;
	size_t  length_max;
	size_t  item_size;
	uint8_t data[];
};

#define array_create(arr) ARR_create(&arr, sizeof(typeof(*arr)))
static inline int ARR_create(void*p, size_t item_size)
{
	size_t max = 2;
	struct ARR_Head *arr = ARR_malloc(sizeof(struct ARR_Head)+item_size*max);
	if (arr==NULL) {
		*(void**)p = NULL;
		return 1;
	}
	arr->length     = 0;
	arr->length_max = max;
	arr->item_size = item_size;

	*(void**)p = arr->data;
	return 0;
}


#define array_delete(arr)  ARR_destroy(&arr)
#define array_destroy(arr) ARR_destroy(&arr)
static inline void ARR_destroy(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	ARR_free(arr);
	*(void**)p = NULL;
};

static inline int ARR_resize(struct ARR_Head **_arr, size_t newlen)
{
	struct ARR_Head *arr = *_arr;
	arr = ARR_realloc(arr, sizeof(struct ARR_Head)+(arr->item_size * newlen));
	if (arr==NULL) 
		return 1;
	arr->length_max = newlen;
	*_arr = arr;
	return 0;
}

#define array_shrink(arr) ARR_shrink(&arr)
static inline int ARR_shrink(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	arr->length_max = arr->length;
	return ARR_resize(&arr, arr->length);
}

#define array_push(arr, ...) ARR_push(&arr,(typeof(*arr)[1]){__VA_ARGS__})
static inline void ARR_push(void*p, const void*data)
{
	struct ARR_Head *arr = ARR_get_header(p);

	while (arr->length+1 > arr->length_max)
		ARR_resize(&arr, arr->length_max*2);

	memcpy( 
		&arr->data[arr->item_size * arr->length],
		data,
		arr->item_size
	);

	arr->length++; 
	*(void**)p = arr->data;
};

#define array_pop(arr) ARR_pop(&arr)
static inline void ARR_pop(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	arr->length--;
}

#define array_length(arr) ARR_length(&arr)
static inline size_t ARR_length(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	return arr->length;
}


#define array_sizeof(arr) ARR_sizeof(&arr)
static inline size_t ARR_sizeof(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	return arr->length * arr->item_size;
}


/* Allocate and add a number of elements, but do not write anything. 
 * Pointer to allocated block is returned. 
 * NOTE: Return pointer may become invalid if any reallocating array functions, 
 * like array_push, are called. */
#define array_reserve(arr, count) ((typeof(arr))ARR_reserve(&arr, count))
static inline void *ARR_reserve(void*p, size_t count)
{
	struct ARR_Head *arr = ARR_get_header(p);

	if (arr->length+count > arr->length_max) {
		size_t new_length = arr->length_max;
		while(new_length < arr->length+count)
			new_length *= 2;
		ARR_resize(&arr, new_length);
	}

	void*ret = arr->data + (arr->item_size*arr->length);
	arr->length += count;

	*(void**)p = arr->data;
	return ret;
}

#define array_clear(arr) ARR_clear(&arr)
static inline void ARR_clear(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	arr->length = 0;
}


#define array_back(arr) ( (typeof(arr))ARR_back(&arr) )
static inline void *ARR_back(void*p)
{
	struct ARR_Head *arr = ARR_get_header(p);
	return arr->data + (arr->length-1)*arr->item_size;
}

#define array_swap(arr, a, b) ARR_swap(&arr, a, b)
static inline void ARR_swap(void*p, size_t a, size_t b)
{
	struct ARR_Head *arr = ARR_get_header(p);
	
	uint8_t tmp[arr->item_size];

	memcpy(
		tmp,
		arr->data + a * arr->item_size,
		arr->item_size
	);
	
	memcpy(
		arr->data + a * arr->item_size,
		arr->data + b * arr->item_size,
		arr->item_size
	);

	memcpy(
		arr->data + b * arr->item_size,
		tmp,
		arr->item_size
	);
}


