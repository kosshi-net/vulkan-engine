#include "phash.h"
#include "sbm.h"
#include "engine/mem/mem.h"
#include "engine/mem/arr.h"
#include <assert.h>

#include <omp.h>

struct PHash *phash_create(float spacing, uint32_t object_count)
{
	struct PHash *this = mem_calloc(sizeof(*this));

	this->spacing = spacing * 2.0;
	this->object_count = object_count;

	this->table_root       = 64;
	this->table_root_shift = __builtin_ctz(this->table_root);
	this->table_cells      = powf(this->table_root, 3);

	this->head = mem_malloc((this->table_cells) * sizeof(uint32_t));
	this->tail = mem_malloc((this->table_cells) * sizeof(uint32_t));

	this->list = mem_malloc((this->object_count) * sizeof(uint32_t));

	this->pair = mem_malloc((this->object_count*64) * sizeof(uint32_t));

	return this;
}

/* This is like actually slow wtf */
static inline uint32_t umod(int32_t i, int32_t n)
{
	return (i % n + n) % n;
}

static inline uint32_t umod2(int32_t i, int32_t n)
{
	return ((uint32_t)i & n-1);
}

static inline uint32_t phash_table_index(struct PHash *restrict this, int32_t v[3])
{
	uint32_t n = this->table_root - 1;
	uint32_t x = v[0]&n;
	uint32_t y = v[1]&n;
	uint32_t z = v[2]&n;
	
	uint32_t shift = this->table_root_shift;

	uint32_t index = (((z << shift) | y) << shift) | x;
	return index;
}


static void phash_int_pos(struct PHash *restrict this, float v[3], int32_t r[3])
{
	r[0] = floorf(v[0] / this->spacing);
	r[1] = floorf(v[1] / this->spacing);
	r[2] = floorf(v[2] / this->spacing);
}

uint32_t phash_pos(struct PHash *this, float *v)
{
	int32_t t[3];
	phash_int_pos(this, v, t);
	
	uint32_t index = phash_table_index(this, t);
	return index;
}



void phash_update(struct PHash *restrict this, float *restrict pos)
{
	memset(this->head, 0xFF, mem_size(this->head));
	memset(this->list, 0xFF, mem_size(this->list));

	for (uint32_t i = 0; i < this->object_count; i++) {
		uint32_t index = phash_pos(this, &pos[i*3]);
		
		uint32_t *head = &this->head[index];
		uint32_t *tail = &this->tail[index];

		if (*head == UINT32_MAX)
			*head = i;
		 else
			this->list[*tail] = i;
		
		*tail = i;
	}
}


static void phash_query_parallel(
	struct PHash *restrict this, 
	float *restrict pos, 
	uint32_t key,
	float radius,
	float radius2,
	uint32_t *query,
	uint32_t *query_count)
{
	float *pos1 = pos+key*3;
	float vmin[3] = M_SUB_VS(pos1, radius);
	float vmax[3] = M_ADD_VS(pos1, radius);

	int32_t min[3];
	int32_t max[3];

	phash_int_pos(this, vmin, min);
	phash_int_pos(this, vmax, max);


	int32_t iv[3];
	for (iv[2] = min[2]; iv[2] <= max[2]; iv[2]++) 
	for (iv[1] = min[1]; iv[1] <= max[1]; iv[1]++) 
	for (iv[0] = min[0]; iv[0] <= max[0]; iv[0]++) 
	{
		uint32_t index = phash_table_index(this, iv);

		uint32_t key2 = this->head[index];

		while (key2 != UINT32_MAX) {
			if (key >= key2) goto skip;

			float *pos2 = pos+key2*3;
			float dist2 = m_distance_squared(pos1, pos2);

			if (dist2 > radius2) goto skip;
			
			query[query_count[0]++] = key;
			query[query_count[0]++] = key2;
			skip:
			key2 = this->list[key2];
		}
	}
}


#define THREADS 6
void phash_query_all(struct PHash *restrict this, float *restrict pos, float radius)
{
	static uint32_t *query[THREADS];

	static int once = 1;
	if (once) {
		for (size_t i = 0; i < THREADS; i++) 
			query[i] = malloc(sizeof(uint32_t) * 2048*16);

		once--;
	}

	float radius2 = radius*radius;

	uint32_t  query_count[THREADS] = {0};

	#pragma omp parallel num_threads(THREADS) 
	{
		uint32_t count = 0;
		int t = omp_get_thread_num();

		#pragma omp for
		for (uint32_t i = 0; i < this->object_count; i++) {

			phash_query_parallel(this, 
				pos, i, 
				radius, radius2, 
				query[t], &count
				//query[t], &query_count[t]
			);
		}

		query_count[t] = count;
	}

	this->pair_count = 0;

	for (size_t t = 0; t < THREADS; t++) {
		memcpy(
			&this->pair[this->pair_count], 
			query[t],
			query_count[t] * sizeof(uint32_t)
		);
		this->pair_count += query_count[t];
	}

	
	/*

	for (uint32_t i = 0; i < this->object_count; i++) {
		uint32_t i1 = i; 
		
		this->adj_first[i1] = this->adj_count;

		phash_query_parallel(this, pos, i1, radius, radius2, this->query, &this->query_count);
	
		for (uint32_t j = 0; j < this->query_count; j++) {
			uint32_t i2 = this->query[j];
			this->adj[this->adj_count++] = i2;
		}
	}
	this->adj_first[this->object_count] = this->adj_count;
	*/
}

