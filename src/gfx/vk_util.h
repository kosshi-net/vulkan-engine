#pragma once
#include "common.h"
#include "gfx_types.h"

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

void vk_copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

VkCommandBuffer vk_begin_commands();
void vk_end_commands(VkCommandBuffer cmdbuf);
