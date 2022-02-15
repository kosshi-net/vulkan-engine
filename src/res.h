#pragma once 
#include "common.h"

enum Resource {
	RES_NONE,
	RES_TEXTURE_TEST,
	RES_SHADER_FRAG_TEST,
	RES_SHADER_VERT_TEST,
};

void *res_file(enum Resource, size_t *);


