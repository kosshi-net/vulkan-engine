#pragma once

#include <cglm/cglm.h>

struct Camera {

	mat4 projection;

	mat4 view_rotation;
	mat4 view_translation;
	mat4 view;

};


