#include "icosphere.h"
#include "common.h"

#include "m_vec3.h"
#include "icosahedron.h"


static void project_to_sphere(struct IcosphereMesh *this)
{
	for (size_t i = 0; i < array_length(this->vertex); i++) {
		float *v = this->vertex[i].pos;
		m_normalize(v);
	}
}

void icosphere_tessellate(struct IcosphereMesh *this)
{
	struct IcosphereFace *new_face;
	array_create(new_face);

	struct {
		uint16_t edge[5][2];
	} dict[ array_sizeof(this->vertex) ];
	memset(dict, 0xFF, sizeof(dict));

	for (size_t i = 0; i < array_length(this->face); i++) {

		uint16_t *C = this->face[i].index;
		uint16_t  E[3];

		for (size_t e = 0; e < 3; e++) {
		
			uint16_t iA = MIN(C[e], C[(e+1)%3]);
			uint16_t iB = MAX(C[e], C[(e+1)%3]);

			uint16_t subkey = 0;
			for (; subkey < 6; subkey++) {
				if (dict[iA].edge[subkey][0] == UINT16_MAX
				 || dict[iA].edge[subkey][0] == iB ) 
					break;
			}

			if (dict[iA].edge[subkey][0] == UINT16_MAX) {
				/* Create new vertex */
				float *A = this->vertex[iA].pos;
				float *B = this->vertex[iB].pos;
				
				dict[iA].edge[subkey][0] = iB;
				dict[iA].edge[subkey][1] = array_length(this->vertex);

				struct IcosphereVertex *vert = array_reserve(this->vertex, 1);
				
				m_add_vvv(vert->pos, A, B);
				m_div_vs (vert->pos, 2.0);
			} 

			E[e] = dict[iA].edge[subkey][1];
		}

		/*   	  C0
		 *  	  /\
		 *  	 /  \
		 *    E2/----\E0
		 *     / \  / \
		 *    /___\/___\
		 *  C2    E1    C1
		 */
		
		array_push(new_face, (struct IcosphereFace){.index={C[0],E[0],E[2]}});
		array_push(new_face, (struct IcosphereFace){.index={C[1],E[1],E[0]}});
		array_push(new_face, (struct IcosphereFace){.index={C[2],E[2],E[1]}});
		array_push(new_face, (struct IcosphereFace){.index={E[0],E[1],E[2]}});
	}
	array_destroy(this->face);
	this->face = new_face;

	project_to_sphere(this);

	this->face_count   = array_length(this->face);
	this->vertex_count = array_length(this->vertex);
}

struct IcosphereMesh *icosphere_create(uint32_t lod)
{
	size_t ico_vertex_size;
	float *ico_vertex;
	icosahedron_vertex(&ico_vertex, &ico_vertex_size);

	size_t ico_index_size;
	uint16_t *ico_index;
	icosahedron_index(&ico_index, &ico_index_size);

	struct IcosphereMesh *this = calloc(1, sizeof(*this));

	array_create(this->vertex);
	array_create(this->face);

	size_t j = 0;

	array_reserve(this->vertex, ico_vertex_size/sizeof(float)/3);
	for (size_t i = 0; i < array_length(this->vertex); i++) {
		this->vertex[i].pos[0] = ico_vertex[j++];
		this->vertex[i].pos[1] = ico_vertex[j++];
		this->vertex[i].pos[2] = ico_vertex[j++];
	}
	
	array_reserve(this->face, ico_index_size/sizeof(uint16_t)/3);
	j = 0;
	for (size_t i = 0; i < array_length(this->face); i++) {
		this->face[i].index[0] = ico_index[j++];
		this->face[i].index[1] = ico_index[j++];
		this->face[i].index[2] = ico_index[j++];
	}

	for (uint32_t i = 0; i < lod; i++) {
		icosphere_tessellate(this);
	}

	this->face_count   = array_length(this->face);
	this->vertex_count = array_length(this->vertex);

	return this;
}

void icosphere_destroy(struct IcosphereMesh **this)
{
	array_destroy(this[0]->vertex);
	array_destroy(this[0]->face);

	free(*this);
	*this = NULL;
}
