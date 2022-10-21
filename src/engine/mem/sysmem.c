#include "mem.h"

static size_t used = 0;

void mem_set_error_callback( void(*fn)(enum MEM_ERROR) )
{
	return;
}


int mem_init(size_t bytes)
{
	return 0;
}

void mem_debug(void)
{
	return;
}


void *mem_alloc(size_t bytes)
{
	size_t *p = malloc(bytes + sizeof(size_t));
	p[0] = bytes;
	used += bytes;
	return &p[1];
}

void *mem_calloc(size_t bytes)
{
	size_t *p = calloc(1, bytes + sizeof(size_t));
	p[0] = bytes;
	used += bytes;
	return &p[1];
}


void *mem_free(void*_p)
{
	size_t *p = _p;
	p--;
	used -= p[0];
	free(p);
	return NULL;
}

void *mem_resize(void *_p, size_t bytes)
{
	size_t *p = _p;
	p--;
	used -= p[0];
	p = realloc(p, sizeof(size_t) + bytes);
	used += bytes;
	p[0] = bytes;
	return &p[1];
}

size_t mem_size(void*_p)
{
	size_t *p = _p;
	return p[-1];
}

size_t mem_used(void)
{
	return used;
}
