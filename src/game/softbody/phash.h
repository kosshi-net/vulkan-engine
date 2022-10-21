#pragma once 

#include "common.h"

struct PHash {

	float spacing;
	
	uint32_t object_count;

	uint32_t *head;
	uint32_t *tail;
	uint32_t  table_root;
	uint32_t  table_root_shift;
	uint32_t  table_cells;


	uint32_t *list;


	uint32_t *pair;
	uint32_t  pair_count;

};





struct PHash *phash_create(float spacing, uint32_t max_objects);

uint32_t phash_int(struct PHash *this, int32_t v[3]);
uint32_t phash_pos(struct PHash *this, float *v);
void phash_update(struct PHash *this, float *pos);
void phash_query_all(struct PHash *this, float *pos, float radius);

