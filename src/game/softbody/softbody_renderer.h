#pragma once 

#include "common.h"
#include "gfx/gfx_types.h"

#include "softbody.h"


typedef Handle SoftbodyRenderer;



SoftbodyRenderer  softbody_renderer_create (void);
void              softbody_renderer_destroy(SoftbodyRenderer*);

void softbody_geometry_prepare(
		SoftbodyRenderer handle, 
		struct SBMesh *restrict mesh);

void softbody_geometry_update(
		SoftbodyRenderer handle,
		struct SBMesh *restrict mesh, 
		struct Frame *restrict frame);


void softbody_renderer_draw(SoftbodyRenderer, struct Frame*);
void softbody_renderer_draw_shadowmap(
	SoftbodyRenderer handle,
	struct Frame *frame);
