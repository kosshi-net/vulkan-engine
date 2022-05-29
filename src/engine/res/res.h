#pragma once 
#include "common.h"

enum Resource {
	RES_NONE,
	RES_TEAPOT,
	RES_TEXTURE_TEST,
	RES_SHADER_FRAG_TEST,
	RES_SHADER_VERT_TEST,
	RES_SHADER_FRAG_TEXT,
	RES_SHADER_VERT_TEXT,
	RES_SHADER_FRAG_WIDGET,
	RES_SHADER_VERT_WIDGET,
};

void *res_file(enum Resource, size_t *);


