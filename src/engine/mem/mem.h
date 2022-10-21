#pragma once 

#include <stdint.h>
#include <stdlib.h>

enum MEM_ERROR {
	MEM_ERROR_UNKNOWN,
	MEM_ERROR_OUT_OF_MEMORY,
	MEM_ERROR_CORRUPTION,
};

void    mem_set_error_callback( void(*)(enum MEM_ERROR) );

int     mem_init(size_t);

#define mem_malloc mem_alloc
void   *mem_alloc(size_t);
void   *mem_calloc(size_t);
void   *mem_free(void*);

#define mem_realloc mem_resize
void   *mem_resize(void *, size_t);

size_t  mem_size(void*);
size_t  mem_used(void);

void    mem_debug(void);
