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
	for (int i = 0; i < array_length(vk.instance_ext_avbl); i++) {
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
		for( int i = 0; i < array_length(vk.instance_ext_avbl); i++ ){
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
	for (int i = 0; i < array_length(vk.validation_avbl); i++) {
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
		for( int i = 0; i < array_length(vk.validation_avbl); i++ ){
			const char *name = vk.validation_avbl[i].layerName;
			log_debug("%s", name );
		}
}

void vk_validation_add(const char *ext)
{
	array_push(vk.validation_req, ext);
}

/************
 * INSTANCE *
 ************/

void vk_create_instance(void)
{
	/* Add GLFW extensions */
	uint32_t glfw_ext_num = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_num);
	for(int i = 0; i < glfw_ext_num; i++)
		vk_instance_ext_add(glfw_ext[i]);

	for (int i = 0; i < array_length(vk.instance_ext_req); i++)
		if (!vk_instance_ext_check(vk.instance_ext_req[i])) 
			engine_crash("Instance extension requirements not met");
		
	for (int i = 0; i < array_length(vk.validation_req); i++)
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
}

