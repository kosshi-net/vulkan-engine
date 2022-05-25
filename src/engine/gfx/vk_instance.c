#include "gfx/gfx_types.h"
#include "engine.h"
#include "common.h"
#include "log/log.h"
#include "array.h"

extern struct VkEngine vk;

/**************
 * EXTENSIONS *
 **************/

bool vk_instance_ext_check(const char*ext)
{
	for (size_t i = 0; i < array_length(vk.instance_ext_avbl); i++) {
		const char *name = vk.instance_ext_avbl[i].extensionName;
		if (strcmp(ext, name)==0) 
			goto found;
	}
	log_error("CHECK %s: FAIL", ext);
	return false;
found:
	log_info("CHECK %s: OK", ext);
	return true;
}


void vk_instance_ext_get_avbl(void)
{
	array_create(vk.instance_ext_req);
	array_create(vk.instance_ext_avbl);

	uint32_t num;

	vkEnumerateInstanceExtensionProperties(NULL, &num, NULL);

	void*data = array_reserve(vk.instance_ext_avbl, num);
	vkEnumerateInstanceExtensionProperties(NULL, &num, data);

	if(vk._verbose)
		for (size_t i = 0; i < array_length(vk.instance_ext_avbl); i++ ){
			const char *name = vk.instance_ext_avbl[i].extensionName;
			log_debug("%s", name );
		}
}

void vk_instance_ext_add(const char *ext)
{
	array_push(vk.instance_ext_req, ext);
}

/**************
 * VALIDATION *
 **************/

bool vk_validation_check(const char*ext)
{
	for (size_t i = 0; i < array_length(vk.validation_avbl); i++) {
		const char *name = vk.validation_avbl[i].layerName;
		if (strcmp(ext, name)==0) goto found;
	}
	log_error("CHECK %s: FAIL\n", ext);
	return false;
found:
	log_info("CHECK %s: OK", ext);
	return true;
}


void vk_validation_get_avbl(void)
{
	array_create(vk.validation_req);
	array_create(vk.validation_avbl);
	uint32_t num;

	vkEnumerateInstanceLayerProperties(&num, NULL);

	void*data = array_reserve(vk.validation_avbl, num);
	vkEnumerateInstanceLayerProperties(&num, data);

	if (vk._verbose)
		for (size_t i = 0; i < array_length(vk.validation_avbl); i++ ){
			const char *name = vk.validation_avbl[i].layerName;
			log_debug("%s", name );
		}
}

void vk_validation_add(const char *ext)
{
	array_push(vk.validation_req, ext);
}


static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
){
	static struct {
		int32_t  message_id;
		uint32_t pass;
		char    *warn;
	} filter[] = {
		{
			.message_id = 0x7cd0911d,
			.warn       = 
				"Known Vulkan issue. "
				"This message will be filtered.",
			.pass       = 1,
		}
	};

	ufast32_t f = 0;
	for (f = 0; f < LENGTH(filter); f++) {
		if (filter[f].message_id == pCallbackData->messageIdNumber) {
			if (filter[f].pass == 0) {
				return VK_FALSE;
			} else {
				filter[f].pass--;
				break;
			}
		}
	}

	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		log_debug("%s", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		log_info("%s", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		log_warn("%s", pCallbackData->pMessage);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		log_error("%s", pCallbackData->pMessage);
		break;
	}

	if (filter[f].message_id == pCallbackData->messageIdNumber
	 && filter[f].pass == 0)
	{
		log_warn(filter[f].warn);
	}

    return VK_FALSE;
}

void vk_destroy_messenger()
{
	PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = 
		(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			vk.instance, "vkDestroyDebugUtilsMessengerEXT"
		);

	if (pfnDestroyDebugUtilsMessengerEXT == NULL)
		engine_crash("pfnDestroyDebugUtilsMessengerEXT failed to load");

	pfnDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, NULL);
}

void vk_create_messenger()
{
    PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = 
		(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			vk.instance, "vkCreateDebugUtilsMessengerEXT"
		);

	if(pfnCreateDebugUtilsMessengerEXT == NULL)
		engine_crash("pfnCreateDebugUtilsMessengerEXT failed to load");

	VkDebugUtilsMessengerCreateInfoEXT debug_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			0,
		.messageType = 
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			0,
		.pfnUserCallback = vk_debug_callback,
		.pUserData = NULL,
	};

	VkResult ret;
	ret = pfnCreateDebugUtilsMessengerEXT(
		vk.instance, 
		&debug_info,
		NULL,
		&vk.messenger
	);
	if (ret != VK_SUCCESS) 
		engine_crash("pfnCreateDebugUtilsMessengerEXT failed");

}

/************
 * INSTANCE *
 ************/

void vk_destroy_instance(void) 
{
	if (vk.debug) vk_destroy_messenger();
	vkDestroyInstance(vk.instance, NULL);
}

void vk_create_instance(void)
{
	/* Add GLFW extensions */
	uint32_t glfw_ext_num = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_num);
	for (ufast32_t i = 0; i < glfw_ext_num; i++)
		vk_instance_ext_add(glfw_ext[i]);

	for (size_t i = 0; i < array_length(vk.instance_ext_req); i++)
		if (!vk_instance_ext_check(vk.instance_ext_req[i])) 
			engine_crash("Instance extension requirements not met");
		
	for (size_t i = 0; i < array_length(vk.validation_req); i++)
		if (!vk_validation_check(vk.validation_req[i])) 
			engine_crash("Validation layer requirements requirements not met");
		

	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = "Vulkan Engine",
		.pEngineName        = "No Engine",
		.apiVersion         = VK_API_VERSION_1_2,
		.applicationVersion = VK_MAKE_VERSION(1,0,0),
		.engineVersion      = VK_MAKE_VERSION(1,0,0),
	};

	
	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo        = &app_info,
		.enabledLayerCount       = array_length(vk.validation_req),
		.ppEnabledLayerNames     = vk.validation_req,
		.enabledExtensionCount   = array_length(vk.instance_ext_req),
		.ppEnabledExtensionNames = vk.instance_ext_req,
	};

	VkResult ret = vkCreateInstance(&create_info, NULL, &vk.instance);
	
	if (ret != VK_SUCCESS) engine_crash("vkCreateInstance failed");

	if (vk.debug) vk_create_messenger();
}

