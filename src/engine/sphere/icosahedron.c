#include "common.h"
#include <math.h>

#define PHI 1.618033988749895 

void icosahedron_vertex(float**out, size_t *size)
{
	const float a = 1.0f;
	const float b = 1.0f / PHI;

	static float vertex[] = {
		 0,  b, -a,
		 b,  a,  0,
		-b,  a,  0,
		 0,  b,  a,
		 0, -b,  a,
		-a,  0,  b,
		 0, -b, -a,
		 a,  0, -b,
		 a,  0,  b,
		-a,  0, -b,
		 b, -a,  0,
		-b, -a,  0
	};


	*out = vertex;
	*size = sizeof(vertex);

}

void icosahedron_index(uint16_t**out, size_t *size)
{
	static uint16_t index[] = {
		2,  1,  0,
		1,  2,  3,
		5,  4,  3,
		4,  8,  3,
		7,  6,  0,
		6,  9,  0,
		11, 10, 4,
		10, 11, 6,
		9,  5,  2,
		5,  9,  11,
		8,  7,  1,
		7,  8,  10,
		2,  5,  3,
		8,  1,  3,
		9,  2,  0,
		1,  7,  0,
		11, 9,  6,
		7,  10, 6,
		5,  11, 4,
		10, 8,  4
	};

	*out = index;
	*size = sizeof(index);
}
