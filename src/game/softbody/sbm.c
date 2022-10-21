#include "sbm.h"

float sbm_ray_sphere(
	float center[3],
	float r,
	float origin[3],
	float ray [3])
{
	float m[3] = M_SUB_VV(origin, center);
	
	float b = m_dot(m, ray);
	float c = m_dot(m, m) - r*r;

	if (c > 0.0f && b > 0.0f) return 0.0;

	float discr = b*b-c;

	if (discr < 0.0f) return 0;

	float t = -b - sqrt(discr);

	return t;
}
