#pragma once

#include "gfx/gfx_types.h"

int engine_init(void);
void engine_destroy(void);
void engine_tick(void);

void engine_wait_idle(void);

struct Frame {
	double          delta;
	struct VkFrame *vk;
};

struct Frame *
frame_begin();

void 
frame_end(struct Frame *);


