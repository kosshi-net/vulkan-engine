#pragma once

#include "common.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <cglm/cglm.h>

#define VK_MAX_FRAMES_IN_FLIGHT 3


struct SceneUBO {
	mat4 view;
	mat4 proj;
};

struct ObjectUBO {
	mat4 model;
};


struct Frame {
	VkSemaphore      image_available;
	VkSemaphore      render_finished;
	VkFence          flight;
	
	VkCommandPool    cmd_pool;
	VkCommandBuffer  cmd_buf;

	uint32_t         object_num;
	VkDescriptorSet *object_descriptors;
	VkDescriptorSet  object_descriptor_dynamic;
	VkBuffer         object_buffer;
	VmaAllocation    object_alloc;

	VkDescriptorSet  descriptor_set;
	VkBuffer         uniform_buffer;
	VmaAllocation    uniform_alloc;
};

struct VkEngine {
	const char                    *error;
	bool                           _verbose;
	GLFWwindow                    *window;

	double                         last_resize;

	VmaAllocator                   vma;

	VkInstance                     instance;
	VkSurfaceKHR                   surface;

	VkDevice                       dev;
	VkPhysicalDevice               dev_physical;
	VkPhysicalDeviceProperties     dev_properties;

	struct Frame                   frames[VK_MAX_FRAMES_IN_FLIGHT];
	
	// Non-rendering only
	VkCommandPool   cmd_pool;      


	uint32_t                       family_graphics;
	bool                           family_graphics_valid;

	uint32_t                       family_presentation;
	bool                           family_presentation_valid;


	VkSwapchainKHR                 swapchain;
	VkImage                       *swapchain_img;
	VkImageView                   *swapchain_img_view;
	uint32_t                       swapchain_img_num;
	VkFormat                       swapchain_img_format;
	VkExtent2D                     swapchain_extent;

	VkImage                        depth_image;
	VmaAllocation                  depth_alloc;
	VkImageView                    depth_view;

	VkDescriptorSetLayout          descriptor_set_layout;
	VkDescriptorSetLayout          object_descriptor_layout;
	VkDescriptorPool               descriptor_pool;

	VkPipelineLayout               pipeline_layout;
	VkRenderPass                   render_pass;
	VkPipeline                     pipeline;
	uint32_t                       framebuffers_num;
	VkFramebuffer                 *framebuffers;
	bool                           framebuffer_resize;

	VkQueue                        graphics_queue;
	VkQueue                        present_queue;

	VkFence                       *fence_image;
	uint32_t                       current_frame;

	VkBuffer                       vertex_buffer;
	VmaAllocation                  vertex_alloc;

	VkBuffer                       index_buffer;
	VmaAllocation                  index_alloc;

	VkBuffer                       staging_buffer;
	VmaAllocation                  staging_alloc;


	VkImage                        texture_image;
	VkSampler                      texture_sampler;
	VkImageView                    texture_view;
	VmaAllocation                  texture_alloc;

	VkExtensionProperties         *instance_ext_avbl;
	const char                   **instance_ext_req;

	VkExtensionProperties         *device_ext_avbl;
	const char                   **device_ext_req;

	VkLayerProperties             *validation_avbl;
	const char                   **validation_req;


};

