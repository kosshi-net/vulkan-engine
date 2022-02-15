#include "vk_util.h"

#include "common.h"
#include "gfx.h"
#include "gfx_types.h"

extern struct VkEngine vk;


/****************
 * MEMORY STUFF *
 ****************/

void vk_create_image_vma(
	uint32_t width, uint32_t height,
	VkFormat              format, 
	VkImageTiling         tiling,
	VkImageUsageFlags     usage,
	VmaMemoryUsage        memory_usage,
	VkImage              *image,
	VmaAllocation        *alloc
){
	if(vk.error) return;

	VkImageCreateInfo image_info = {
		.sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType       = VK_IMAGE_TYPE_2D,
		.extent          = { .width = width, .height = height, .depth = 1 },
		.mipLevels       = 1,
		.arrayLayers     = 1,
		.format          = format,
		.tiling          = tiling,
		.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage           = usage,
		.sharingMode     = VK_SHARING_MODE_EXCLUSIVE,
		.samples         = VK_SAMPLE_COUNT_1_BIT,
		.flags           = 0,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = memory_usage
	};

	VkResult ret = vmaCreateImage(vk.vma, &image_info, &alloc_info, image, alloc, NULL);
	if(ret){
		vk.error = "Texture image creation failed (vma)";
		return;
	}
}

void vk_create_buffer_vma(
	VkDeviceSize          size, 
	VkBufferUsageFlags    usage, 
	VmaMemoryUsage        memory_usage,
	VkBuffer             *buffer, 
	VmaAllocation        *alloc
){
	if(vk.error) return;
	VkResult ret;

    VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = memory_usage,
	};
	
	ret = vmaCreateBuffer(vk.vma, &buffer_info, &alloc_info, buffer, alloc, NULL);
    if (ret != VK_SUCCESS) {
		vk.error = "Failed to allocate (vma)";
		return;
    }
}


void vk_copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer cmdbuf = vk_begin_commands();

	VkBufferCopy copy_info = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size      = size,
	};
	vkCmdCopyBuffer(cmdbuf, src, dst, 1, &copy_info);

	vk_end_commands(cmdbuf);
}

/************
 * COMMANDS *
 ************/


VkCommandBuffer vk_begin_commands()
{
	VkCommandBufferAllocateInfo alloc_info = {
		.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = vk.cmd_pool,
		.commandBufferCount = 1,
	};
	VkCommandBuffer cmdbuf;
	vkAllocateCommandBuffers(vk.dev, &alloc_info, &cmdbuf);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(cmdbuf, &begin_info);
	return cmdbuf;
}

void vk_end_commands(VkCommandBuffer cmdbuf)
{
	vkEndCommandBuffer(cmdbuf);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdbuf,
	};

	vkQueueSubmit(vk.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk.graphics_queue);

	vkFreeCommandBuffers(vk.dev, vk.cmd_pool, 1, &cmdbuf);
}


