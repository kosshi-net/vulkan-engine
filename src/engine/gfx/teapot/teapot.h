#pragma once

#include "gfx/gfx.h"
#include "engine.h"

struct TeapotRenderer {

	struct TeapotFrameData {
		uint32_t         object_num;
		VkBuffer         object_buffer;
		VmaAllocation    object_alloc;


		VkDescriptorSet  scene_descriptor;
		VkDescriptorSet  object_descriptor;

		VkBuffer         uniform_buffer;
		VmaAllocation    uniform_alloc;
	} frame[VK_FRAMES];

	VkDescriptorSetLayout          scene_layout;
	VkDescriptorSetLayout          object_layout;

	VkPipelineLayout               pipeline_layout;
	VkPipeline                     pipeline;

	VkBuffer                       vertex_buffer;
	VmaAllocation                  vertex_alloc;

	VkBuffer                       index_buffer;
	VmaAllocation                  index_alloc;

	VkImage                        texture_image;
	VkImageView                    texture_view;
	VmaAllocation                  texture_alloc;

};

uint32_t gfx_teapot_renderer_create(void);
void     gfx_teapot_draw(struct Frame *);
void     gfx_teapot_renderer_destroy(uint32_t);
