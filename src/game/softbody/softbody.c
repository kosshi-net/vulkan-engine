#include "softbody.h"
#include "softbody_renderer.h"
#include "wireframe_renderer.h"
#include "engine/sphere/sphere_renderer.h"
#include "sbm.h"
#include "common.h"
#include "input.h"
#include "freecam.h"
#include <assert.h>

#include "mem/arr.h"

/* In meters, 0.01 = 1 cm */
#define PARTICLE_DIAMETER 0.015
#define PARTICLE_RADIUS (PARTICLE_DIAMETER / 2.0)

#define CLOTH_WIDTH  1.0
#define CLOTH_HEIGHT 2.0

#define CLOTH_VISUAL_THICKNESS 0.0

#define GRAVITY -9.8

#define LENIENCY_STRETCH 0.0
#define LENIENCY_SHEAR   0.001
#define LENIENCY_BEND    0.00005

#define FREQ 60
#define SUBSTEPS 6

struct {
	
	double st_begin[SUBSTEPS];
	double st_end  [SUBSTEPS];

	double st_hash[SUBSTEPS];
	double st_query[SUBSTEPS];

} perf;

void softbody_update_mesh(struct Softbody *restrict this)
{

	struct SBMesh *mesh = &this->mesh;

	memset(mesh->normal, 0,         mem_size(mesh->normal));
	//memcpy(mesh->vertex, this->pos, mem_size(mesh->vertex));

	for (size_t i = 0; i < mesh->index_side; i+=3) {
		uint32_t p0 = mesh->index[i + 0];
		uint32_t p1 = mesh->index[i + 1];
		uint32_t p2 = mesh->index[i + 2];

		vec3 e1 = M_SUB_VV(this->pos[p0], this->pos[p1]);
		vec3 e2 = M_SUB_VV(this->pos[p0], this->pos[p2]);

		vec3 normal; 

		m_cross    (normal, e1, e2);
		m_normalize(normal);

		m_add_vv(this->normal[p0], normal);
		m_add_vv(this->normal[p1], normal);
		m_add_vv(this->normal[p2], normal);

	}
	for (size_t i = 0; i < this->particle_count; i++) {
		m_normalize(this->normal[i]);
		uint32_t j = i + mesh->vertex_side;
		m_set_vv (mesh->normal[i], this->normal[i]); 
		m_set_vvd(mesh->normal[j], this->normal[i], 1.0); 


		m_set_vv( mesh->vertex[i], this->pos[i] );
		m_set_vv( mesh->vertex[j], this->pos[i] );


		float r = this->radius * CLOTH_VISUAL_THICKNESS;
		m_add_vvd( mesh->vertex[i], this->normal[i],  r );
		m_add_vvd( mesh->vertex[j], this->normal[i], -r );

		//m_add_vvd(v.pos, v.normal, this->radius * 0.5);
	}


}


void softbody_prepare_mesh(struct Softbody *restrict this, 
		int32_t w, int32_t h)
{

	struct SBMesh *mesh = &this->mesh;

	/* Triangulate  */

	this->mesh.quad = mem_malloc(sizeof(struct SBQuad)*32);
	for (int32_t y = 1; y < h; y++) 
	for (int32_t x = 1; x < w; x++)
	{

		/*
		 *  p2 ----- p1   p2 ----- p1   p2 ----- p1
		 *  |       / |   | \       |   | \     / |
		 *  |    /    |   |    \    |   |    X    |
		 *  | /       |   |       \ |   | /     \ |
		 *  p4 ----- p3   p4 ----- p3   p4 ----- p3
		 *
		 * */

		uint32_t p1 = y*w + x;
		uint32_t p2 = p1 - 1;
		uint32_t p3 = (y-1)*w + x;
		uint32_t p4 = p3 - 1;

		arr_push(this->mesh.quad, (struct SBQuad){
			.corner = { p2, p1, p3, p4 },
		});
	}

	mesh->edge = mem_alloc(sizeof(struct SBEdge)*32);
	for (int32_t x = 0; x < w-1; x++) {
		arr_push(mesh->edge, (struct SBEdge){.pair = {x, x+1}});

		uint32_t p = ((h-1)*w)+x;
		arr_push(mesh->edge, (struct SBEdge){.pair = {p, p+1}});
	}

	this->mesh.vertex_count = this->particle_count * 2.0;
	this->mesh.vertex = mem_alloc(this->mesh.vertex_count * sizeof(vec3));
	this->mesh.normal = mem_alloc(this->mesh.vertex_count * sizeof(vec3));


	/*
	 *  0  -----  1   0  -----  1 
	 *  |       / |   | \       | 
	 *  |    /    |   |    \    | 
	 *  | /       |   |       \ | 
	 *  3  -----  2   3  -----  2 
	 * */

	this->mesh.index_count = 0;
	this->mesh.index = mem_alloc(sizeof(uint16_t)*32);

	for (size_t i = 0; i < this->mesh.quad_count; i++) {
		struct SBQuad *quad = &this->mesh.quad[i];

		arr_push(this->mesh.index, quad->corner[0]);
		arr_push(this->mesh.index, quad->corner[1]);
		arr_push(this->mesh.index, quad->corner[3]);
		arr_push(this->mesh.index, quad->corner[2]);
		arr_push(this->mesh.index, quad->corner[3]);
		arr_push(this->mesh.index, quad->corner[1]);
		arr_push(this->mesh.index, quad->corner[0]);
		arr_push(this->mesh.index, quad->corner[1]);
		arr_push(this->mesh.index, quad->corner[2]);
		arr_push(this->mesh.index, quad->corner[2]);
		arr_push(this->mesh.index, quad->corner[3]);
		arr_push(this->mesh.index, quad->corner[0]);
	}

	/* Clone index buffer*/
	mesh->vertex_side = mesh->vertex_count/2;
	mesh->index_side  = mesh->index_count;

	for (uint32_t i = 0; i < mesh->index_side; i++) {
		arr_push(mesh->index, mesh->index[i] + mesh->vertex_side);
	}
}

struct Softbody *
cloth_create_towel(void)
{
	struct Softbody *this = mem_calloc(sizeof(*this));



	this->diameter = PARTICLE_DIAMETER ;
	this->radius   = PARTICLE_RADIUS   ;

	float spacing = PARTICLE_DIAMETER * 0.8;

	const int32_t w = CLOTH_WIDTH  / spacing;
	const int32_t h = CLOTH_HEIGHT / spacing;

	log_info("vec3 size: %li", sizeof(vec3));

	this->particle_count = w*h;

	this->pos      = mem_alloc (this->particle_count*sizeof(vec3));
	this->pos_prev = mem_calloc(this->particle_count*sizeof(vec3));
	this->pos_rest = mem_calloc(this->particle_count*sizeof(vec3));
	this->vel      = mem_calloc(this->particle_count*sizeof(vec3));
	this->normal   = mem_calloc(this->particle_count*sizeof(vec3));
	
	this->mass_inv = mem_calloc(this->particle_count*sizeof(float));

	for (int32_t y = 0; y < h; y++)
	for (int32_t x = 0; x < w; x++) 
	{
		uint32_t i = y*w+x;
		this->pos[i][0] = x*spacing;
		this->pos[i][1] = y*spacing;
		this->pos[i][2] = m_randf()* PARTICLE_DIAMETER * 0.00;

	}

	memcpy(this->pos_rest, this->pos, mem_size(this->pos_rest));

	/* Transform */
	{
		mat4 m;
		glm_mat4_identity(m);
		glm_rotate_x(m, glm_rad(3), m);
		//glm_rotate_x(m, glm_rad(90), m);
		glm_rotate_z(m, glm_rad(20), m);
		//glm_rotate_x(m, glm_rad(10), m);
		//glm_rotate_y(m, glm_rad(180), m);
		for (uint32_t i = 0; i < this->particle_count; i++) {
			float *p = this->pos[i];
			vec4 v = {p[0], p[1], p[2], 0.0};
			vec4 out;
			glm_mul(m, &v, &out);
			
			p[0] = out[0] - CLOTH_WIDTH / 2.0;
			p[1] = out[1] + 1.0;
			p[2] = out[2] - CLOTH_HEIGHT / 2.0;
		}
	}


	for (uint32_t i = 0; i < this->particle_count; i++) {
		this->mass_inv[i] = 1.0;
	}

	/* Constraints */
	{
		uint32_t *fiber_A = mem_alloc(sizeof(uint32_t) * 32);
		uint32_t *fiber_B = mem_alloc(sizeof(uint32_t) * 32);

		float *fiber_leniency = mem_alloc(sizeof(float) * 32);

		uint32_t  fiber_A_count = 0;
		uint32_t  fiber_B_count = 0;
		uint32_t  fiber_leniency_count = 0;
		


		if(1)
		for (int32_t y = 1; y < h; y++) 
		for (int32_t x = 1; x < w; x++)
		{


			uint32_t p1 = y*w + x;
			uint32_t p2 = p1 - 1;
			uint32_t p3 = (y-1)*w + x;
			uint32_t p4 = p3 - 1;
			
			arr_push(fiber_A, p1);
			arr_push(fiber_B, p4);
			arr_push(fiber_leniency, LENIENCY_SHEAR);
			arr_push(fiber_A, p2);
			arr_push(fiber_B, p3);
			arr_push(fiber_leniency, LENIENCY_SHEAR);
		}

		if(1)
		for (int32_t y = 1; y < h-1; y++) 
		for (int32_t x = 1; x < w-1; x++)
		{
			uint32_t p0 = y*w+x;
			uint32_t p1 = p0 - 1;
			uint32_t p2 = p0 + 1;

			uint32_t p3 = p0 - w;
			uint32_t p4 = p0 + w;

			arr_push(fiber_A, p1);
			arr_push(fiber_B, p2);
			arr_push(fiber_leniency, LENIENCY_BEND);
			arr_push (fiber_A, p3);
			arr_push (fiber_B, p4);
			arr_push (fiber_leniency, LENIENCY_BEND);
		}

		for (int32_t y = 0; y < h; y++)
		for (int32_t x = 1; x < w; x++) {
			uint32_t p1 = y*w+x;
			uint32_t p2 = p1 - 1;
			arr_push(fiber_A, p1);
			arr_push(fiber_B, p2);
			arr_push(fiber_leniency, LENIENCY_STRETCH);
		}

		for (int32_t x = 0; x < w; x++) 
		for (int32_t y = 1; y < h; y++){
			uint32_t p1 = y*w+x;
			uint32_t p2 = p1 - w;
			arr_push(fiber_A, p1);
			arr_push(fiber_B, p2);
			arr_push(fiber_leniency, LENIENCY_STRETCH);
		}

		this->fiber_count = fiber_A_count;

		this->fiber_A        = fiber_A;
		this->fiber_B        = fiber_B; 
		this->fiber_leniency = fiber_leniency;

		this->fiber_length   = mem_alloc(this->fiber_count*sizeof(float));
	}

	for (uint32_t i = 0; i < this->fiber_count; i++) {
		this->fiber_length[i] = m_distance(
			this->pos[this->fiber_A[i]],
			this->pos[this->fiber_B[i]]
		);
	}

	for (uint32_t i = 0; i < this->pin_count; i++) {
		this->mass_inv[ this->pin_p[i] ] = 0.0;
	}

	this->hash = phash_create(this->diameter, this->particle_count);

	softbody_prepare_mesh(this, w, h);

	log_info("Cloth vertices: %i", this->particle_count);
	log_info("Cloth triangles: %i", this->mesh.index_count/3);

	return this;
}



void softbody_solve_fibers(struct Softbody *restrict this, float delta)
{
	for (uint32_t i = 0; i < this->fiber_count; i++) {

		//log_info("fiber %i %i", this->fiber_A[i], this->fiber_B[i]);


		uint32_t p1 = this->fiber_A[i];
		uint32_t p2 = this->fiber_B[i];


		float alpha = this->fiber_leniency[i] / delta / delta; // ??

		float w1 = this->mass_inv[p1];
		float w2 = this->mass_inv[p2];
		float w  = w1+w2;

		if (w == 0.0) continue;

		float v[3] = M_SUB_VV( this->pos[p1], this->pos[p2] );

		float len = m_length(v);
		
		if (len == 0.0) continue;

		m_mul_vs(v, 1.0 / len);

		float restlen = this->fiber_length[i];

		float C = len - restlen;
		float s = -C / (w+alpha);
		
		m_add_vvd(this->pos[p1], v,  s * w1);
		m_add_vvd(this->pos[p2], v, -s * w2);
	}
}

void softbody_solve_tether(struct Softbody *restrict this, uint32_t p1, float delta)
{
	float alpha = 0.0;
	for (uint32_t p2 = 0; p2 < this->particle_count; p2++) {

		float w1 = this->mass_inv[p1];
		float w2 = this->mass_inv[p2];
		float w  = w1+w2;

		if (w == 0.0) continue;

		float v[3] = M_SUB_VV( this->pos[p1], this->pos[p2] );

		float len = m_length(v);
		
		if (len == 0.0) continue;

		m_mul_vs(v, 1.0 / len);

		float restlen = m_distance( this->pos_rest[p1], this->pos_rest[p2] ) * 1.2;

		float C = len - restlen;
		if(C < 0.0) continue;
		float s = -C / (w+alpha);
		
		m_add_vvd(this->pos[p2], v, -s * w2);
	}
}

void softbody_solve_collisions(struct Softbody *restrict this, float delta)
{
	struct PHash *restrict hash = this->hash;
	float diameter2 = this->diameter * this->diameter;

	uint32_t counter = 0;

	for (uint32_t i = 0; i < hash->pair_count; i+=2) {

		uint32_t p1 = hash->pair[i];
		uint32_t p2 = hash->pair[i+1];
		
		counter++;
		if (this->mass_inv[p1] == 0.0) continue;
		if (this->mass_inv[p2] == 0.0) continue;

		float diff[3] = M_SUB_VV(this->pos[p2], this->pos[p1]);
		
		float dist2 = m_length_squared(diff);
		if (dist2 > diameter2 || dist2 == 0.0)
			continue;

		float rest2 = m_distance_squared(this->pos_rest[p2], this->pos_rest[p1]);
		float min = this->diameter;

		if (dist2 > rest2)
			continue;

		if (rest2 < diameter2)
			min = sqrtf(rest2);

		/* Correct position */
		float dist = sqrtf(dist2);
		m_mul_vs(diff, (min - dist) / dist);

		m_add_vvd(this->pos[p1], diff, -0.5);
		m_add_vvd(this->pos[p2], diff, +0.5);

		/* Friction */
		
		/* Velocity */
		float v1[3], v2[3], v3[3];

		m_sub_vvv(v1, this->pos[p1], this->pos_prev[p1]);
		m_sub_vvv(v2, this->pos[p2], this->pos_prev[p2]);

		/* Average velocity */
		m_add_vvv(v3, v1, v2);
		m_mul_vs(v3, 0.5);

		m_sub_vvv(v1, v3, v1);
		m_sub_vvv(v2, v3, v2);

		float friction = 0.5;
		m_add_vvd(this->pos[p1], v1, friction);
		m_add_vvd(this->pos[p2], v2, friction);
	}
}


void softbody_solve_ground(struct Softbody *restrict this)
{
	for (uint32_t i = 0; i < this->particle_count; i++) {
		if (this->mass_inv[i] == 0.0) continue;

		float y = this->pos[i][1];
		float min = this->radius;
		if (y < min) {
			float damping = 0.5;
			float v[3] = M_SUB_VV(this->pos[i], this->pos_prev[i]);
			m_add_vvd(this->pos[i], v, -damping);
			this->pos[i][1] = min;
		}
	}
}


static struct {
	struct Softbody *softbody;
	WireframeRenderer gfx_wireframe;
	SoftbodyRenderer  gfx_softbody;
	SphereRenderer    gfx_sphere;

	bool              pause;
	uint32_t          step;

	bool              wireframe;
	bool              sphere;

	bool     cast;
	uint32_t grab;
	float    dist;
	float    grab_mass;
} sb;



void softbody_create()
{
	sb.softbody = cloth_create_towel();
	mem_debug();
	softbody_update_mesh(sb.softbody);
	mem_debug();
	
	sb.gfx_softbody  = softbody_renderer_create();
	sb.gfx_wireframe = wireframe_renderer_create();
	sb.gfx_sphere    = sphere_renderer_create();
	softbody_geometry_prepare(sb.gfx_softbody, &sb.softbody->mesh);

	sb.pause = true;
	sb.wireframe = true;
	
	sb.grab = UINT32_MAX;

	log_info("Softbody created");
	
	float F = FREQ*SUBSTEPS;
	
	float frame_delta = 1.0 / (float)FREQ;
	float delta = frame_delta / (float)SUBSTEPS;
	float vmax = (0.5 * sb.softbody->diameter / delta);
	float dmax = vmax * frame_delta;

	log_info("%.2f Hz %.2fcm vmax %.4f", 
		F,
		PARTICLE_DIAMETER*100,
		vmax
	); 

	log_info("vmax %.2f dmax %.2f", vmax, dmax);

	log_info("%.2f m/s", vmax);
}

static float gravity[3] = {
	0,
	GRAVITY,
	0
};

void softbody_simulate(struct Softbody *this)
{
	static const double frame_delta = 1.0 / (float)FREQ;
	static double last = 0;
	double now = glfwGetTime();

	if (last+frame_delta < now) {
		last += frame_delta;
		float delta = frame_delta / (float)SUBSTEPS;

		float vmax = (0.25 * this->diameter * (float)(FREQ*SUBSTEPS));
		float dmax = vmax * frame_delta;
		//float dmax = vmax / (float)SUBSTEPS;
		
		bool pause = sb.pause && !sb.step;

		if(!pause && 0){
			double _t1, _t2; 
			_t1 = glfwGetTime();
			phash_update(this->hash,    (void*)this->pos);
			
			_t2 = glfwGetTime();
			perf.st_hash[0] = _t2 - _t1;

			phash_query_all(this->hash, (void*)this->pos, this->diameter*1.0);

			_t1 = glfwGetTime();
			perf.st_query[0] = _t1 - _t2;
		}
		
		if(!pause)
		for (ufast32_t substep = 0; substep < SUBSTEPS; substep++) {
	
			if(sb.step) 
				sb.step--;
			else if (sb.pause)
				break;
			
			perf.st_begin[substep] = glfwGetTime();

			/* Integrate */
#pragma omp parallel for num_threads(6)
			for (uint32_t i = 0; i < this->particle_count; i++) {
				if (this->mass_inv[i] == 0) continue;
			
				vec3 *vel = this->vel;
				vec3 *pos = this->pos;
				vec3 *prev = this->pos_prev;

				m_add_vvd(vel[i], gravity, delta);
	
				float v = m_length(vel[i]);
				if (v > vmax)
					m_mul_vs(vel[i], vmax/v);

				m_set_vv(prev[i], pos[i]);
				m_add_vvd(pos[i], vel[i], delta);
				
			}


			if(1){
				double _t1, _t2; 
				_t1 = glfwGetTime();
				phash_update(this->hash,    (void*)this->pos);
				
				_t2 = glfwGetTime();
				perf.st_hash[substep] = _t2 - _t1;

				phash_query_all(this->hash, (void*)this->pos, this->diameter*1.1);

				_t1 = glfwGetTime();
				perf.st_query[substep] = _t1 - _t2;
			}

			softbody_solve_collisions(this, delta);

			uint32_t tether = UINT32_MAX;
			if(1)
			for (uint32_t i = 0; i < this->particle_count; i++) {
				if (this->mass_inv[i] == 0) {
					tether = i;
					softbody_solve_tether(this, tether, delta);
					break;
				}
			}
			for (size_t i = 0; i < 1; i++) {
				softbody_solve_fibers(this, delta);
			}



			softbody_solve_ground(this);



#pragma omp parallel for num_threads(6)
			for (uint32_t i = 0; i < this->particle_count; i++) {
				if (this->mass_inv[i] == 0.0) continue;
				m_sub_vvv( this->vel[i], this->pos[i], this->pos_prev[i] );
				m_mul_vs( this->vel[i], 1.0/delta );
			}


			perf.st_end[substep] = glfwGetTime();
		}
		//softbody_update_normals(this);
		softbody_update_mesh(this);
	}
	
	if(!sb.pause)
	{
		double total;
		double total_hash;
		double total_query;
		for (size_t i = 0; i < SUBSTEPS; i++) {
			total += perf.st_end[i] - perf.st_begin[i];

			total_query += perf.st_query[i];
			total_hash += perf.st_hash[i];

		}
		log_info( "Simulation) total: %.2f, substep avg: %.2f, hash %.2f, query %.2f" ,
			total*1000.0,
			total/(float)SUBSTEPS * 1000.0,
			total_hash * 1000.0,
			total_query * 1000.0
		);
	}
	if (now-last > 1.0) {
		log_warn("Can't keep up (%f %f)!", last, now);
		last = now;
	}
}

uint32_t softbody_cast(struct Softbody *this, float origin[3], float ray[3], float *dist)
{
	uint32_t id = UINT32_MAX;
	float    best = INFINITY;
	for (uint32_t i = 0; i < this->particle_count; i++) {
		
		float d = sbm_ray_sphere(this->pos[i], this->diameter, origin, ray);

		if (d == 0.0) continue;
		
		if (best > d) {
			best = d;
			id = i;
		}
	}
	*dist = best;
	return id;
}


void softbody_update(struct Frame *frame, struct Freecam *cam)
{
	if (sb.cast) {
		if (sb.grab == UINT32_MAX) {
			sb.grab = softbody_cast(sb.softbody, cam->pos, cam->dir, &sb.dist);
			if (sb.grab != UINT32_MAX) {
				log_info("Grabbed %u %f", sb.grab, sb.dist);

				sb.dist = m_distance(cam->pos, sb.softbody->pos[sb.grab]);

				sb.grab_mass = sb.softbody->mass_inv[sb.grab];
				sb.softbody->mass_inv[sb.grab] = 0.0;
			}
		} else {
			float v[3];
			m_mul_vvs(v, cam->dir, sb.dist);
			m_add_vv(v, cam->pos);

			m_set_vv(sb.softbody->pos[sb.grab], v);
		}
	} else if (sb.grab != UINT32_MAX) {

		sb.softbody->mass_inv[sb.grab] = sb.grab_mass;
		sb.grab = UINT32_MAX;

	}
	
	softbody_simulate(sb.softbody);
}

void softbody_predraw(struct Frame *frame, struct Freecam *cam)
{
	softbody_renderer_draw_shadowmap(sb.gfx_softbody, frame);
}

void softbody_draw(struct Frame *frame, struct Freecam *cam)
{
	softbody_geometry_update(sb.gfx_softbody, &sb.softbody->mesh, frame);

	struct Softbody *this = sb.softbody;
	struct SBMesh *mesh = &this->mesh;

	if(sb.wireframe){
		for (uint32_t i = 0; i < this->mesh.index_count; i+=3) {
			wireframe_triangle(sb.gfx_wireframe,
				this->mesh.vertex[ this->mesh.index[i+0] ],
				this->mesh.vertex[ this->mesh.index[i+1] ],
				this->mesh.vertex[ this->mesh.index[i+2] ],
				0x00FFFFFF
			);
		}

		for (uint32_t i = 0; i < mesh->edge_count; i++) {
			wireframe_line(sb.gfx_wireframe, 
				this->pos[ mesh->edge[i].pair[0] ],
				this->pos[ mesh->edge[i].pair[1] ],
				0xFFFF00FF
			);
		}
	}
	int32_t n = 5;

	for (int32_t i = -n; i < n+1; i++) {
		wireframe_line(sb.gfx_wireframe, 
			(float[]){ n, 0, i},
			(float[]){-n, 0, i},
		0xFF3333FF);
		wireframe_line(sb.gfx_wireframe, 
			(float[]){i, 0, n},
			(float[]){i, 0,-n},
		0x3333FFFF);
	}

	/* Sphere renderer */
	if(sb.sphere) {
	srand(0);
		for (uint32_t i = 0; i < sb.softbody->particle_count; i++) {

			float hue = (i / (float)sb.softbody->particle_count) * 360.0;
			float rgb[3];
			hsv_to_rgb(hue, 1.0, 1.0, rgb);
			
			uint32_t color;
			color |= (uint32_t)(rgb[0] * 255.0);
			color <<= 8;
			color |= (uint32_t)(rgb[1] * 255.0);
			color <<= 8;
			color |= (uint32_t)(rgb[2] * 255.0);

			sphere_renderer_add(sb.gfx_sphere, sb.softbody->pos[i], 
					sb.softbody->radius,
					color << 8 | 0xFF);
		}
	}
	if (sb.grab != UINT32_MAX) {

			sphere_renderer_add(sb.gfx_sphere, sb.softbody->pos + (sb.grab), 
					sb.softbody->radius,
					0xFF0000FF);


	}

	sphere_renderer_draw(sb.gfx_sphere, frame);


	wireframe_renderer_draw(sb.gfx_wireframe, frame);
	softbody_renderer_draw(sb.gfx_softbody, frame);
}

void softbody_scroll_callback(double x, double y)
{

	if (sb.cast) {
		sb.dist += 0.03 * y;
	}

}

void softbody_button_callback(int key, int action, int mods)
{
	if (key == GLFW_MOUSE_BUTTON_LEFT)  {
	
		if (action == GLFW_PRESS) {
			sb.cast = true;
		} else {
			sb.cast = false;
		}
	}


}

void softbody_key_callback(int key, int scancode, int action, int mods)
{
	if (action != GLFW_RELEASE) return;
	
	switch (key) {
	case (GLFW_KEY_P):
		sb.pause = !sb.pause;
		log_info("Toggle simulation");
		break;
	case (GLFW_KEY_BACKSLASH):
		sb.step += SUBSTEPS;
		break;
	case (GLFW_KEY_RIGHT_BRACKET):
		sb.step += 1;
		break;
	case (GLFW_KEY_LEFT_BRACKET):
		sb.step += 1;
		break;
	case (GLFW_KEY_F):
		log_info("Toggle wireframe");
		sb.wireframe = !sb.wireframe;
		break;
	case (GLFW_KEY_G):
		log_info("Toggle sphere");
		sb.sphere = !sb.sphere;
		break;
	}
}
