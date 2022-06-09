#pragma once

#include "common.h"

struct IcosphereVertex {
	float pos[3];
};

struct IcosphereFace {
	uint16_t index[3];
};  

struct IcosphereMesh {
	struct IcosphereVertex *vertex;
	size_t vertex_count; 

	struct IcosphereFace *face;
	size_t face_count; 
};

struct IcosphereMesh *
icosphere_create(uint32_t lod);

void 
icosphere_tessellate(struct IcosphereMesh *);

void 
icosphere_destroy(struct IcosphereMesh **);
