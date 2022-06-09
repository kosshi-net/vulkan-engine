#pragma once

#include <math.h>

#define M_ADD_VV(v1, v2) {v1[0]+v2[0], v1[1]+v2[1], v1[2]+v2[2]}
#define M_SUB_VV(v1, v2) {v1[0]-v2[0], v1[1]-v2[1], v1[2]-v2[2]}
#define M_DIV_VV(v1, v2) {v1[0]/v2[0], v1[1]/v2[1], v1[2]/v2[2]}
#define M_MUL_VV(v1, v2) {v1[0]*v2[0], v1[1]*v2[1], v1[2]*v2[2]}

#define M_ADD_VS(v1, s)  {v1[0]+s, v1[1]+s, v1[2]+s}
#define M_SUB_VS(v1, s)  {v1[0]-s, v1[1]-s, v1[2]-s}
#define M_DIV_VS(v1, s)  {v1[0]/s, v1[1]/s, v1[2]/s}
#define M_MUL_VS(v1, s)  {v1[0]*s, v1[1]*s, v1[2]*s}

static inline void m_set_vv(
	float *dst,
	float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}


/* ADD */

static inline void m_add_vvv(
	float *dst,
	float *v1,
	float *v2)
{
	dst[0] = v1[0] + v2[0];
	dst[1] = v1[1] + v2[1];
	dst[2] = v1[2] + v2[2];
}

static inline void m_add_vv(
	float *v1,
	float *v2)
{
	v1[0] += v2[0];
	v1[1] += v2[1];
	v1[2] += v2[2];
}

static inline void m_add_vvd(
	float *v1,
	float *v2,
	float delta)
{
	v1[0] += v2[0]*delta;
	v1[1] += v2[1]*delta;
	v1[2] += v2[2]*delta;
}

static inline void m_add_vvs(
	float *dst,
	float *v1,
	float s)
{
	dst[0] = v1[0] + s;
	dst[1] = v1[1] + s;
	dst[2] = v1[2] + s;
}

static inline void m_add_vs(
	float *v1,
	float s)
{
	v1[0] += s;
	v1[1] += s;
	v1[2] += s;
}




/* SUB */

static inline void m_sub_vvv(
	float *dst,
	float *v1,
	float *v2)
{
	dst[0] = v1[0] - v2[0];
	dst[1] = v1[1] - v2[1];
	dst[2] = v1[2] - v2[2];
}

static inline void m_sub_vv(
	float *v1,
	float *v2)
{
	v1[0] -= v2[0];
	v1[1] -= v2[1];
	v1[2] -= v2[2];
}

static inline void m_sub_vvs(
	float *dst,
	float *v1,
	float s)
{
	dst[0] = v1[0] - s;
	dst[1] = v1[1] - s;
	dst[2] = v1[2] - s;
}

static inline void m_sub_vs(
	float *v1,
	float s)
{
	v1[0] -= s;
	v1[1] -= s;
	v1[2] -= s;
}


/* MUL */

static inline void m_mul_vvv(
	float *dst,
	float *v1,
	float *v2)
{
	dst[0] = v1[0] * v2[0];
	dst[1] = v1[1] * v2[1];
	dst[2] = v1[2] * v2[2];
}

static inline void m_mul_vv(
	float *v1,
	float *v2)
{
	v1[0] *= v2[0];
	v1[1] *= v2[1];
	v1[2] *= v2[2];
}

static inline void m_mul_vvs(
	float *dst,
	float *v1,
	float s)
{
	dst[0] = v1[0] * s;
	dst[1] = v1[1] * s;
	dst[2] = v1[2] * s;
}

static inline void m_mul_vs(
	float *v1,
	float s)
{
	v1[0] *= s;
	v1[1] *= s;
	v1[2] *= s;
}



/* DIV */

static inline void m_div_vvv(
	float *dst,
	float *v1,
	float *v2)
{
	dst[0] = v1[0] / v2[0];
	dst[1] = v1[1] / v2[1];
	dst[2] = v1[2] / v2[2];
}

static inline void m_div_vv(
	float *v1,
	float *v2)
{
	v1[0] /= v2[0];
	v1[1] /= v2[1];
	v1[2] /= v2[2];
}

static inline void m_div_vvs(
	float *dst,
	float *v1,
	float s)
{
	dst[0] = v1[0] / s;
	dst[1] = v1[1] / s;
	dst[2] = v1[2] / s;
}

static inline void m_div_vs(
	float *v1,
	float s)
{
	v1[0] /= s;
	v1[1] /= s;
	v1[2] /= s;
}



/* CROSS PRODUCT */

static inline void m_cross(
	float *dst,
	float *v1,
	float *v2)
{
	dst[0] = v1[1] * v2[2] - v1[2] * v2[1]; 
	dst[1] = v1[2] * v2[0] - v1[0] * v2[2];
	dst[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

#define M_CROSS(v1, v2) {\
	r[0] = v1[1] * v2[2] - v1[2] * v2[1], \
	r[1] = v1[2] * v2[0] - v1[0] * v2[2], \
	r[2] = v1[0] * v2[1] - v1[1] * v2[0]}

/* DOT PRODUCT */

static inline float m_dot(float *v1, float *v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

/* SQUARE ROOTS */

static inline float m_length(float *v){
	float x = v[0];
	float y = v[1];
	float z = v[2];
	return sqrtf( x*x + y*y + z*z );
}

static inline float m_length_squared(float *v){
	float x = v[0];
	float y = v[1];
	float z = v[2];
	return x*x + y*y + z*z;
}

static inline void m_normalize(float *v)
{
	float l = m_length(v);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static inline float m_distance(float *p1, float *p2){
	float v[3] = M_SUB_VV(p1, p2);
	m_mul_vv(v, v);
	return sqrtf( v[0] + v[1] + v[2] );
}

static inline float m_distance_squared(float *p1, float *p2){
	float v[3] = M_SUB_VV(p1, p2);
	m_mul_vv(v, v);
	return v[0] + v[1] + v[2];
}



#include <stdlib.h>

static inline float m_urandf(void){
	return (rand()/(float)RAND_MAX);
}

static inline float m_randf(void){
	return (rand()/(float)RAND_MAX)*2.0-1.0;
}

