#pragma once 

#include "common.h"
#include "freecam.h"
#include "phash.h"

struct SBPoint {
	float pos[3];
	float last[3];

	float vel[3];

	float normal[3];
};



struct SBQuad {
	uint32_t corner[4];
	uint32_t center;
	struct {
		float normal[4];
	} tri[4];
};

struct SBEdge {
	uint32_t pair[2];
};

struct SBMesh {
	uint16_t  *index;
	uint32_t   index_count;
	vec3      *vertex;
	uint32_t   vertex_count;
	vec3      *normal;

	uint32_t   vertex_side;
	uint32_t   index_side;

	struct SBQuad    *quad;
	uint32_t          quad_count;

	struct SBEdge    *edge;
	uint32_t          edge_count;
};



struct Softbody {

	uint32_t particle_count;

	vec3  *pos;
	vec3  *pos_prev;
	vec3  *pos_rest;
	vec3  *vel;
	float *mass_inv;
	vec3  *normal; /* Visual, computed from triangles */

	float radius;
	float diameter;

	/* Constraints */

	uint32_t *fiber_A;
	uint32_t *fiber_B;
	float    *fiber_leniency;
	float    *fiber_length;
	uint32_t  fiber_count;

	uint32_t *pin_p;
	uint32_t  pin_count;

	/* Mesh */

	struct SBMesh mesh;

	struct PHash *hash;
};

struct Vert {
	float *pos;
	float *normal;
};

void softbody_create(void);
void softbody_update(struct Frame *frame, struct Freecam *cam);

void softbody_predraw(struct Frame *frame, struct Freecam *cam);
void softbody_draw(struct Frame *frame, struct Freecam *cam);

void softbody_key_callback(int key, int scancode, int action, int mods);
void softbody_button_callback(int key, int action, int mods);
void softbody_scroll_callback(double x, double y);
