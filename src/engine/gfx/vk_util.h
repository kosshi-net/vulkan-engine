#pragma once
#include "common.h"
#include "gfx_types.h"
#include "res.h"

void vk_create_image_vma(
	uint32_t width, uint32_t height,
	VkFormat              format, 
	VkImageTiling         tiling,
	VkImageUsageFlags     usage,
	VmaMemoryUsage        memory_usage,
	VkImage              *image,
	VmaAllocation        *alloc
);

void vk_create_buffer_vma(
	VkDeviceSize          size, 
	VkBufferUsageFlags    usage, 
	VmaMemoryUsage        memory_usage,
	VkBuffer             *buffer, 
	VmaAllocation        *alloc
);

void vk_copy_buffer(
	VkBuffer src, 
	VkBuffer dst, 
	VkDeviceSize size
);

void vk_upload_buffer(
	VkBuffer      *buffer, 
	VmaAllocation *alloc,
	void          *data,
	size_t         size,
	enum VkBufferUsageFlagBits usage
);

VkCommandBuffer vk_begin_commands();

void vk_end_commands(
	VkCommandBuffer cmdbuf
);




VkFormat vk_find_supported_format( 
	VkFormat *formats,
	size_t    formats_num,
	VkImageTiling tiling, 
	VkFormatFeatureFlags features 
);
	
bool vk_has_stencil(VkFormat format);

VkFormat vk_find_depth_format();


VkShaderModule vk_create_shader_module(enum Resource file);


void vk_transition_image_layout(
	VkImage       image,
	VkFormat      format,
	VkImageLayout old_layout,
	VkImageLayout new_layout
);


void vk_copy_buffer_to_image(
	VkBuffer buffer,
	VkImage  image,
	uint32_t width,
	uint32_t height
);
