#include "array.h"
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct Array {
	size_t  length;
	size_t  length_max;
	size_t  item_size;
	uint8_t data[];
};
typedef struct Array* Array;

void*array_create(size_t item_size)
{
	size_t max = 2;
	struct Array *arr = calloc(sizeof(struct Array)+item_size*max, 1);
	arr->length     = 0;
	arr->length_max = max;
	arr->item_size = item_size;
	return arr->data;
}

Array d2a(void *data)
{
	return &((Array)data)[-1];
}

void array_resize(Array *_arr, size_t newlen)
{
	Array arr = *_arr;
	arr = realloc(arr, sizeof(struct Array)+(arr->item_size * newlen));
	arr->length_max = newlen;
	*_arr = arr;
}

void*array_reserve(void*p, size_t count)
{
	Array arr = &(*(Array*)p)[-1] ;
	if( arr->length+count > arr->length_max ){
		size_t new_length = arr->length_max;
		while(new_length < arr->length+count)
			new_length *= 2;
		array_resize(&arr, new_length);
	}

	void*ret = arr->data + (arr->item_size*arr->length);
	arr->length += count;

	*(void**)p = arr->data;
	return ret;
}

void array_push(void*p, const void*data)
{
	Array arr = &(*(Array*)p)[-1] ;

	while( arr->length+1 > arr->length_max )
		array_resize(&arr, arr->length_max*2);

	memcpy( 
		&arr->data[arr->item_size * arr->length],
		data,
		arr->item_size
	);

	arr->length++; 
	*(void**)p = arr->data;
}

void array_destroy(void*p)
{
	Array arr = &(*(Array*)p)[-1] ;
	free(arr);
	*(void**)p = NULL;
}

size_t array_length(void*p)
{
	Array arr = &(*(Array*)p)[-1] ;
	return arr->length;
}

