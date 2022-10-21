#include "common.h"
#include "freecam.h"
#include "input.h"
#include "gfx/camera.h"

void freecam_update(
	struct Freecam *restrict freecam, 
	double delta)
{
	/* Rotation */
	const double sens = 0.1;
	double cx, cy;

	input_cursor_delta(&cx, &cy);

	freecam->yaw   += cx * sens;
	freecam->pitch += cy * sens;

	freecam->pitch = MIN(freecam->pitch,  90.0);
	freecam->pitch = MAX(freecam->pitch, -90.0);

	freecam->yaw = fmod(360.0 + freecam->yaw, 360.0);

	float yaw   = glm_rad(-freecam->yaw);
	float pitch = glm_rad(freecam->pitch);

	/* Translation */
	freecam->dir[0] = cosf(pitch) * -sinf(yaw);
	freecam->dir[1] = sinf(-pitch);
	freecam->dir[2] = cosf(pitch) * -cosf(yaw);


	float right[] = {
		-sinf(yaw + M_PI/2.0), 
		0,
		-cosf(yaw + M_PI/2.0)
	};
	float *vec = input_vector();

	float s = delta * 0.5;

	freecam->pos[0] += (freecam->dir[0] * vec[2] + right[0]*vec[0])*s;
	freecam->pos[2] += (freecam->dir[2] * vec[2] + right[2]*vec[0])*s;
	freecam->pos[1] -= vec[1]*s;

	/* Field of view */
	freecam->fov -= input_getkey(GLFW_KEY_Z) * delta * 5.0;
	freecam->fov += input_getkey(GLFW_KEY_X) * delta * 5.0;
}

void freecam_to_camera(
	struct Freecam *restrict freecam, 
	struct Camera  *restrict camera)
{
	glm_mat4_identity(camera->view_rotation);

	mat4 world;
	glm_mat4_identity(world);
	
	glm_translate(world, freecam->pos);
	glm_mat4_inv(world, camera->view_translation);

	glm_rotate(camera->view_rotation, glm_rad(freecam->pitch), (vec3){1, 0, 0});
	glm_rotate(camera->view_rotation, glm_rad(freecam->yaw),   (vec3){0, 1, 0});

	glm_mul(
		camera->view_rotation,
		camera->view_translation,
		camera->view
	);
}

