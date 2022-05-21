#pragma once

#include "gfx/gfx_types.h"
#include "gfx/camera.h"

void engine_init(void);
void engine_destroy(void);
void engine_tick(void);

void engine_wait_idle(void);

struct Frame {
	struct Camera camera;

	double          delta;
	struct VkFrame *vk;

	uint32_t width;
	uint32_t height;
};

struct Frame *
frame_begin();

void frame_end(struct Frame *);

void _engine_crash(const char*, const char*, const char*, int);
#define engine_crash(msg) (_engine_crash)(msg, __FILE__, __func__, __LINE__)


