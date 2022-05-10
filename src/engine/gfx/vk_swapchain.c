#include "common.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_swapchain.h"
#include "gfx/text/vk_text.h"

extern struct VkEngine vk;

/**************
 * SWAP CHAIN *
 **************/


struct VkSwapchainDetails *vk_swapchain_details(VkPhysicalDevice dev){
	if (vk.error) return NULL;
	struct VkSwapchainDetails *swap = calloc(sizeof(struct VkSwapchainDetails), 1);

	// Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, vk.surface, 
		&swap->capabilities);
	
	// Formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(dev, vk.surface, 
		&swap->format_num, NULL
	);
	if(swap->format_num){
		swap->format = malloc( swap->format_num * sizeof(VkSurfaceFormatKHR) );
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, vk.surface, 
			&swap->format_num, swap->format
		);
	}

	// Presentation modes
	vkGetPhysicalDeviceSurfacePresentModesKHR(dev, vk.surface, 
		&swap->pmode_num, NULL
	);
	if(swap->pmode_num){
		swap->pmode = malloc( swap->pmode_num * sizeof(VkPresentModeKHR) );
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, vk.surface, 
			&swap->pmode_num, swap->pmode
		);
	}
	return swap;
}

void vk_swapchain_details_free(struct VkSwapchainDetails **swap){
	if(swap[0]->pmode)  free(swap[0]->pmode);
	if(swap[0]->format) free(swap[0]->format);
	free(swap[0]);
	*swap = NULL;
}



VkSurfaceFormatKHR *vk_choose_swap_format( struct VkSwapchainDetails *swap)
{
	for( int i = 0; i < swap->format_num; i++ ){
		VkSurfaceFormatKHR *format = swap->format+i;
		if( format->format == VK_FORMAT_B8G8R8A8_SRGB
		&&  format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		) return format;

	}
	return swap->format;
}

VkPresentModeKHR vk_choose_present_mode( struct VkSwapchainDetails *swap )
{
	for( int i = 0; i < swap->pmode_num; i++ ){
		if( swap->pmode[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			return swap->pmode[i];
	}
	return  VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vk_choose_extent( struct VkSwapchainDetails *swap )
{
	VkSurfaceCapabilitiesKHR *cpbl = &swap->capabilities;
	if( cpbl->currentExtent.width != UINT32_MAX ){
		return cpbl->currentExtent;
	}else{
		int width, height;

		glfwGetFramebufferSize(vk.window, &width, &height);
		
		VkExtent2D extent = {
			.width = width,
			.height = height,
		};
	
		extent.width = MIN(MAX(
			extent.width,
			cpbl->minImageExtent.width),
			cpbl->maxImageExtent.width
		);

		extent.height = MIN(MAX(
			extent.height,
			cpbl->minImageExtent.height),
			cpbl->maxImageExtent.height
		);

		return extent;
	}
}

void vk_create_swapchain(void){
	if (vk.error) return;

	struct VkSwapchainDetails *swap = vk_swapchain_details(vk.dev_physical);
	
	VkSurfaceFormatKHR surface_format = *vk_choose_swap_format(swap);
	VkPresentModeKHR   pmode          =  vk_choose_present_mode(swap);
	VkExtent2D         extent         =  vk_choose_extent(swap);

	uint32_t image_num =swap->capabilities.minImageCount+1;
	if(swap->capabilities.maxImageCount > 0 )
		image_num = MIN(image_num, swap->capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR swap_create = {
		.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface          = vk.surface,
		.minImageCount    = image_num,
		.imageFormat      = surface_format.format,
		.imageColorSpace  = surface_format.colorSpace,
		.imageExtent      = extent,
		.imageArrayLayers = 1,
		.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform     = swap->capabilities.currentTransform,
		.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode      = pmode,
		.clipped          = VK_TRUE,
		.oldSwapchain     = VK_NULL_HANDLE
	};

	uint32_t qfamilyarr[] = {vk.family_graphics, vk.family_presentation};
	
	if(vk.family_graphics != vk.family_presentation){
		swap_create.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swap_create.queueFamilyIndexCount = 2;
		swap_create.pQueueFamilyIndices = qfamilyarr;
	}else{
		swap_create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swap_create.queueFamilyIndexCount = 0;
		swap_create.pQueueFamilyIndices = NULL;
	}
	
	VkBool32 ret = vkCreateSwapchainKHR(
			vk.dev, 
			&swap_create, 
			NULL, 
			&vk.swapchain
	);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create swapchain";
	};

	vkGetSwapchainImagesKHR(vk.dev, vk.swapchain, &image_num, NULL);
	vk.swapchain_img = malloc(sizeof(VkImage) * image_num);
	vkGetSwapchainImagesKHR(vk.dev, vk.swapchain, &image_num, vk.swapchain_img);
	
	vk.swapchain_img_num = image_num;

	vk.swapchain_img_format = surface_format.format;
	vk.swapchain_extent = extent;
	
}

void vk_create_image_views(void)
{
	if(vk.error) return;
	vk.swapchain_img_view = malloc(sizeof(VkImageView) * vk.swapchain_img_num);
	
	for( int i = 0; i < vk.swapchain_img_num; i++ ){
		VkImageViewCreateInfo create_info = {
			.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image    = vk.swapchain_img[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format   = vk.swapchain_img_format,

			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,

			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
		};
		VkResult ret = vkCreateImageView(vk.dev, &create_info, NULL, &vk.swapchain_img_view[i]);
		if(ret != VK_SUCCESS) {
			vk.error = "Failed to create image view";
			return;
		};
	}
	return;
}

void vk_destroy_swapchain()
{

	vk_text_destroy_fbdeps();
	
	printf("Free pipeline\n");
	vkDestroyPipeline(vk.dev, vk.pipeline, NULL);
	printf("Free pipeline layout\n");
	vkDestroyPipelineLayout(vk.dev, vk.pipeline_layout, NULL);
	printf("Free render pass\n");
    vkDestroyRenderPass(vk.dev, vk.renderpass, NULL);

	printf("Free imgviews\n");
	for(int i = 0; i < vk.swapchain_img_num; i++)
		vkDestroyImageView(vk.dev, vk.swapchain_img_view[i], NULL);
	printf("Free swap\n");
	vkDestroySwapchainKHR(vk.dev, vk.swapchain, NULL);

	printf("Free depth\n");
	vkDestroyImageView(vk.dev, vk.depth_view, NULL);

	vmaDestroyImage(vk.vma, vk.depth_image, vk.depth_alloc);

	printf("Free fbs\n");
	for(int i = 0; i < vk.framebuffers_num; i++)
		vkDestroyFramebuffer(vk.dev, vk.framebuffers[i], NULL);

}

