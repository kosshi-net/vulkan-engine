#pragma once

#include "gfx_types.h"

int gfx_init(void);
void gfx_destroy(void);
void gfx_tick(void);

struct VkFrame *
gfx_frame_get(void);

void 
gfx_frame_submit(struct VkFrame *);

void
gfx_draw_teapots(struct VkFrame *);
