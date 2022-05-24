#include "engine.h"
#include "log/log.h"
#include "gfx.h"
#include "gfx_types.h"
#include "vk_swapchain.h"
#include "vk_device.h"

#include "common.h"

extern struct VkEngine vk;

/**************
 * EXTENSIONS *
 **************/

bool vk_device_ext_check(const char*ext)
{
	for (size_t i = 0; i < array_length(vk.device_ext_avbl); i++ ) {
		const char *name = vk.device_ext_avbl[i].extensionName;
		if (strcmp(ext, name)==0) goto found;
	}
	log_error("CHECK %s: FAIL", ext);
	return false;
found:
	log_info("CHECK %s: OK", ext);
	return true;
}


void vk_device_ext_add(const char *ext)
{
	if (!vk.device_ext_req)
		array_create(vk.device_ext_req);

	array_push(vk.device_ext_req, ext);
}


bool vk_device_ext_satisfied(VkPhysicalDevice dev)
{
	if (vk.device_ext_avbl != NULL) 
		array_delete(vk.device_ext_avbl);

	if (vk.device_ext_req == NULL) {
		engine_crash("vk_dev_ext uninitialized");
		return false;
	}

	uint32_t num;
	vkEnumerateDeviceExtensionProperties(dev, NULL, &num, NULL);

	array_create(vk.device_ext_avbl);
	void*data  = array_reserve(vk.device_ext_avbl, num);

	vkEnumerateDeviceExtensionProperties(dev, NULL, &num, data);
	
	if(vk._verbose)
		for(size_t i = 0; i < array_length(vk.device_ext_avbl); i++)
			log_debug("%s", vk.device_ext_avbl[i].extensionName);

	for (size_t i = 0; i < array_length(vk.device_ext_req); i++ )
		if(!vk_device_ext_check(vk.device_ext_req[i]))
			return false;

	return true;

}

/****************
 * QUEUE FAMILY *
 ****************/

void vk_find_family_indices(VkPhysicalDevice device)
{
	vk.family_graphics_valid     = false;
	vk.family_presentation_valid = false;

	uint32_t family_num = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_num, NULL);
	VkQueueFamilyProperties qfamily[family_num];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_num, qfamily);
	
	for (size_t i = 0; i < family_num; i++) {
		if (qfamily[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			vk.family_graphics = i;
			vk.family_graphics_valid = true;
		}

		VkBool32 can_present = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vk.surface, &can_present);
		if(can_present){
			vk.family_presentation = i;
			vk.family_presentation_valid = true;
		}

	}
}


void vk_select_gpu(void)
{
	uint32_t dev_num = 0;
	vkEnumeratePhysicalDevices(vk.instance, &dev_num, NULL);
	if (dev_num == 0) engine_crash("No devices available");

	VkPhysicalDevice dev[dev_num];
	vkEnumeratePhysicalDevices(vk.instance, &dev_num, dev);
	
	Array(int32_t) valid_gpus;
	array_create(valid_gpus);

	log_info("Devices: %i", dev_num);

	for (uint32_t i = 0; i < dev_num; i++){

		if(!vk_device_ext_satisfied(dev[i]))
			continue;
		
		VkPhysicalDeviceProperties dev_properties;
		vkGetPhysicalDeviceProperties(dev[i], &dev_properties);
		
		VkPhysicalDeviceFeatures dev_features;
		vkGetPhysicalDeviceFeatures(dev[i], &dev_features);

		log_info("%s", dev_properties.deviceName);

		if(!dev_features.samplerAnisotropy){
			continue;
		}

		vk_find_family_indices( dev[i] );

		if (!vk.family_graphics_valid 
		 || !vk.family_presentation_valid
		)   continue;

		//if(dev_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
		//	continue;
		if (!dev_features.geometryShader) 
			continue;

		struct VkSwapchainDetails *swap = vk_swapchain_details(dev[i]);

		int ok = !(swap->format_num == 0 || swap->pmode_num  == 0);
		vk_swapchain_details_free(&swap);
		if (!ok) return;

		log_info("Device OK");
		array_push(valid_gpus, i);

		continue;
	}

	if (array_length(valid_gpus) == 0)
		engine_crash("No suitable device");
	if (array_length(valid_gpus) > 1)
		engine_crash("Can't decide between multiple suitable devices (TODO)");
	if (array_length(valid_gpus) == 1)
		vk.dev_physical = dev[valid_gpus[0]];
	array_destroy(valid_gpus);
}


VkDevice vk_create_device()
{
	vk_select_gpu();
	
	static float qpriority = 1.0;

	Array(VkDeviceQueueCreateInfo) qarray;
	array_create(qarray);

	uint32_t families[] = {vk.family_graphics, vk.family_presentation};
	uint32_t families_num = sizeof(families)/sizeof(uint32_t);
	
	for (uint32_t i = 0; i < families_num; i++) {
		array_push(qarray, (VkDeviceQueueCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = families[i],
			.queueCount = 1,
			.pQueuePriorities = &qpriority,
		});
	}

	VkPhysicalDeviceFeatures device_features = {
		.samplerAnisotropy = VK_TRUE,
	};

	VkDeviceCreateInfo dev_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pQueueCreateInfos       = qarray,
		.queueCreateInfoCount    = array_length(qarray),
		.pEnabledFeatures        = &device_features,
		
		.ppEnabledExtensionNames = vk.device_ext_req,
		.enabledExtensionCount   = array_length(vk.device_ext_req),

		.ppEnabledLayerNames     = vk.validation_req,
		.enabledLayerCount       = array_length(vk.validation_req),
	};

	VkDevice dev = VK_NULL_HANDLE;
	vkCreateDevice(vk.dev_physical, &dev_create_info, NULL, &dev);

	array_delete(qarray);

	vkGetDeviceQueue(dev, vk.family_graphics,     0, &vk.graphics_queue);
	vkGetDeviceQueue(dev, vk.family_presentation, 0, &vk.present_queue);

	vk.dev = dev;

	return dev;
}

