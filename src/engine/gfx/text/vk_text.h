#pragma once 

#include "gfx/gfx_types.h"
#include "gfx/text/text.h"
#include "engine.h"

struct TextUBO {
	mat4 ortho;
};

struct VkTextContext {
	struct TextFrameData {
		VkBuffer         uniform_buffer;
		VmaAllocation    uniform_alloc;
		VkDescriptorSet  descriptor_set;

		VkBuffer         vertex_buffer;
		VmaAllocation    vertex_alloc;
		void            *vertex_mapping;

		uint32_t         index_count;
	} frame[VK_FRAMES];

	struct TextContext            *ctx;

	VkPipelineLayout               pipeline_layout;
	VkPipeline                     pipeline;
	VkDescriptorSetLayout          descriptor_layout;

	VkBuffer                       index_buffer;
	VmaAllocation                  index_alloc;

	VkImage                        texture_image;
	VkImageView                    texture_view;
	VmaAllocation                  texture_alloc;
};

uint32_t gfx_text_renderer_create (struct TextContext *);
void     gfx_text_renderer_destroy(uint32_t);
void     gfx_text_draw            (struct Frame*, uint32_t);

