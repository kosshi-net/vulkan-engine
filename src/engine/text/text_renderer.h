#pragma once 

#include "common.h"
#include "gfx/gfx_types.h"
#include "text/text_common.h"



struct TextRenderer {
	bool valid;

	TextEngine                     engine;

	VkPipelineLayout               pipeline_layout;
	VkPipeline                     pipeline;
	VkDescriptorSetLayout          descriptor_layout;

	VkBuffer                       index_buffer;
	VmaAllocation                  index_alloc;

	VkImage                        texture_image;
	VkImageView                    texture_view;
	VmaAllocation                  texture_alloc;
};

TextRenderer text_renderer_create (TextEngine);
TextRenderer text_renderer_destroy(TextRenderer);
void         text_renderer_draw   (TextRenderer, TextGeometry, struct Frame*);
struct TextRenderer *text_renderer_get_struct(TextEngine handle);
