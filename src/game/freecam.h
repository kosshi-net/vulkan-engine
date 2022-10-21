#pragma once

#include "gfx/camera.h"

struct Freecam {
	float yaw;
	float pitch;
	float dir[3];
	float pos[3];
	float fov;
};

void freecam_update(struct Freecam *, double delta);
void freecam_to_camera(struct Freecam *, struct Camera *);

