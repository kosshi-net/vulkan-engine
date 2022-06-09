#pragma once 

#include "common.h"
#include "gfx/gfx_types.h"

typedef Handle SphereRenderer;

SphereRenderer 
sphere_renderer_create(void);

void 
sphere_renderer_destroy(
	SphereRenderer*);

void
sphere_renderer_draw(
	SphereRenderer, 
	struct Frame*);

void
sphere_renderer_add(
	SphereRenderer,
	float    c[3],
	float    r,
	uint32_t color
);


