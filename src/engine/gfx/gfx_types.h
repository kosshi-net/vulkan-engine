#pragma once

#include "common.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>

#include "array.h"

#define VK_FRAMES 3

struct VkFrame {
	uint32_t         id;

	VkSemaphore      image_available;
	VkSemaphore      render_finished;
	VkFence          flight;
	
	VkCommandPool    cmd_pool;
	VkCommandBuffer  cmd_buf;

	uint32_t         image_index;
};

struct VkEngine {
	GLFWwindow                    *window;

	char dev_name[1024];

	bool _verbose;
	double                         last_resize;

	/* 
	 * Core resources
	 */

	bool                           debug;
	VkDebugUtilsMessengerEXT       messenger;

	VmaAllocator                   vma;

	VkInstance                     instance;
	VkSurfaceKHR                   surface;

	VkDevice                       dev;
	VkPhysicalDevice               dev_physical;
	VkPhysicalDeviceProperties     dev_properties;

	uint32_t                       family_graphics;
	bool                           family_graphics_valid;

	uint32_t                       family_presentation;
	bool                           family_presentation_valid;

	Array(VkExtensionProperties)   instance_ext_avbl;
	Array(const char*)             instance_ext_req;

	Array(VkExtensionProperties)   device_ext_avbl;
	Array(const char*)             device_ext_req;

	Array(VkLayerProperties)       validation_avbl;
	Array(const char*)             validation_req;

	/* 
	 * Swapchain resources
	 */

	struct VkFrame                 frames[VK_FRAMES];

	VkSwapchainKHR                 swapchain;
	VkImage                       *swapchain_img;
	VkImageView                   *swapchain_img_view;
	uint32_t                       swapchain_img_num;
	VkFormat                       swapchain_img_format;
	VkExtent2D                     swapchain_extent;

	VkImage                        depth_image;
	VmaAllocation                  depth_alloc;
	VkImageView                    depth_view;

	uint32_t                       framebuffers_num;
	VkFramebuffer                 *framebuffers;
	bool                           framebuffer_resize;

	VkQueue                        graphics_queue;
	VkQueue                        present_queue;

	VkFence                       *fence_image;
	uint32_t                       current_frame;

	/* 
	 * Subrenderer resources 
	 */

	VkCommandPool                  cmd_pool;  /* Primarily used for staging */

	VkBuffer                       staging_buffer;
	VmaAllocation                  staging_alloc;

	VkRenderPass                   renderpass;
	VkSampler                      texture_sampler;
	VkDescriptorPool               descriptor_pool;
};

