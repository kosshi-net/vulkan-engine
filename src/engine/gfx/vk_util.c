#include "vk_util.h"

#include "common.h"
#include "engine.h"
#include "gfx.h"
#include "gfx_types.h"
#include "res/res.h"

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
	if (ret != VK_SUCCESS) engine_crash("vmaCreateImage failed");
}

void vk_create_buffer_vma(
	VkDeviceSize          size, 
	VkBufferUsageFlags    usage, 
	VmaMemoryUsage        memory_usage,
	VkBuffer             *buffer, 
	VmaAllocation        *alloc
){
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
    if (ret != VK_SUCCESS) engine_crash("vmaCreateBuffer failed");
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

/* Create a vulkan buffer from data */
void vk_upload_buffer(
	VkBuffer      *buffer, 
	VmaAllocation *alloc,
	void          *data,
	size_t         size,
	enum VkBufferUsageFlagBits usage
){
	vk_create_buffer_vma(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&vk.staging_buffer,
		&vk.staging_alloc
	);

	void *mapped;
	vmaMapMemory(vk.vma, vk.staging_alloc, &mapped);
	memcpy(mapped, data, size);
	vmaUnmapMemory(vk.vma, vk.staging_alloc);

	vk_create_buffer_vma(
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
		VMA_MEMORY_USAGE_GPU_ONLY,
		buffer,
		alloc
	);

	vk_copy_buffer(vk.staging_buffer, *buffer, size);
	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);
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

VkFormat vk_find_supported_format( 
	VkFormat *formats,
	size_t    formats_num,
	VkImageTiling tiling, 
	VkFormatFeatureFlags features 
){
	for (int i = 0; i < formats_num; i++){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vk.dev_physical, formats[i], &props);

		if (tiling == VK_IMAGE_TILING_LINEAR 
		&& (props.linearTilingFeatures & features) == features)
			return formats[i];

		if (tiling == VK_IMAGE_TILING_OPTIMAL 
		&& (props.optimalTilingFeatures & features) == features)
			return formats[i];

	}
	engine_crash("No supported formats");
	return 0;
}

bool vk_has_stencil(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT 
		|| format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat vk_find_depth_format()
{
	VkFormat formats[] = {
		VK_FORMAT_D32_SFLOAT, 
		VK_FORMAT_D32_SFLOAT_S8_UINT, 
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	return vk_find_supported_format(
		formats, 
		LENGTH(formats), 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}


VkShaderModule vk_create_shader_module(enum Resource file)
{
	size_t spv_size;
	char  *spv = res_file(file, &spv_size);
	
	if (!spv) engine_crash("Missing spv file");

	VkShaderModuleCreateInfo create_info = {
		.sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spv_size,
		.pCode    = (void*)spv,
	};
	VkShaderModule shader_module;
	VkResult ret  = vkCreateShaderModule(vk.dev, &create_info, NULL, &shader_module);
	free(spv);
	if(ret != VK_SUCCESS) engine_crash("vkCreateShaderModule failed");
	
	return shader_module;
}



/*
 * Images
 */

void vk_transition_image_layout(
	VkImage       image,
	VkFormat      format,
	VkImageLayout old_layout,
	VkImageLayout new_layout
){
	VkCommandBuffer cmdbuf = vk_begin_commands();

	VkImageMemoryBarrier barrier = {
		.sType      = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout  = old_layout,
		.newLayout  = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1,
		},
		// TODO
		.srcAccessMask = 0,
		.dstAccessMask = 0,
	};
	
	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED 
	 && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		src_stage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	else 
	if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
	 && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		src_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		engine_crash("Unsupported layout transition");
	}

	vkCmdPipelineBarrier(
		cmdbuf,
		src_stage, dst_stage,
		0,
		0, NULL,
		0, NULL,
		1, &barrier
	);
	vk_end_commands(cmdbuf);
}

void vk_copy_buffer_to_image(
	VkBuffer buffer,
	VkImage  image,
	uint32_t width,
	uint32_t height
){
	VkCommandBuffer cmdbuf = vk_begin_commands();
	
	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,

		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0,0,0},
		.imageExtent = {
			width,
			height,
			1
		},
	};

	vkCmdCopyBufferToImage(
		cmdbuf,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		1, 
		&region
	);
	
	vk_end_commands(cmdbuf);
}
