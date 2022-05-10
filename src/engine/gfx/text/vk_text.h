#pragma once 

#include "gfx/gfx_types.h"
#include "gfx/text/text.h"


void vk_text_create_pipeline(void);
void vk_text_create_renderpass(void);
void vk_text_create_descriptor_layout(void);

void vk_text_update_uniform_buffer(struct Frame *frame);

void vk_text_create(void);
void vk_text_destroy(void);

void vk_text_create_fbdeps(void);
void vk_text_destroy_fbdeps(void);

void vk_text_frame_create (struct Frame *frame);
void vk_text_frame_destroy(struct Frame *frame);

void vk_text_add_ctx(struct TextContext *ctx);

