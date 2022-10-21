/* Soft body mathemathics */

#include "m_vec3.h"

#include "common.h"
#include "softbody.h"

/*
static inline void 
sbm_triangle_normal( 
	struct SBTriangle *restrict tri,
	struct SBPoint *restrict    points,
	float *restrict normal)
{
	float *p1 = points[tri->p[0]].pos;
	float *p2 = points[tri->p[1]].pos;
	float *p3 = points[tri->p[2]].pos;

	float u[3] = M_SUB_VV(p1, p2);
	float v[3] = M_SUB_VV(p1, p3);

	m_cross(u, v, normal);
	m_normalize(normal);
}
*/

float sbm_ray_sphere(
	float center[3],
	float r,
	float origin[3],
	float ray [3]
);


static void hsv_to_rgb(float H, float S, float V, float rgb[3])
{
	
	float r,g,b;

    double  P, Q, T,
            fract;

    (H == 360.)?(H = 0.):(H /= 60.);
    fract = H - floor(H);

    P = V*(1. - S);
    Q = V*(1. - S*fract);
    T = V*(1. - S*(1. - fract));

    if      (0. <= H && H < 1.){
        r = V; g = T; b = P;
	}else if (1. <= H && H < 2.){
		r = Q; g = V; b = P;
	}else if (2. <= H && H < 3.){
		r = P; g = V; b = T;
	}else if (3. <= H && H < 4.){
		r = P; g = Q; b = V;
	}else if (4. <= H && H < 5.){
		r = T; g = P; b = V;
	}else if (5. <= H && H < 6.){
		r = V; g = P; b = Q;
	}else{
        r = 0.; g = 0.; b = 0.;
	}

	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

