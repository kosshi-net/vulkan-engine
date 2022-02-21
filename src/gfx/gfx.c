#include "gfx.h"
#include "array.h"
#include "fileutil.h"

#include <unistd.h>

#include "common.h"
#include "gfx_types.h"
#include "vk_util.h"
#include "res.h"

#include <vk_mem_alloc.h>
#include "vk_instance.h"
#include "vk_device.h"
#include "vk_swapchain.h"

#include <errno.h>
//#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fast_obj.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>


/***********************
 * NECESSARY FUNC DEFS *
 ***********************/

static VkFormat vk_find_depth_format(void);
void vk_create_depth_resources(void);

/**************
 * TEMP UTILS *
 **************/

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

float urandf(){
	return (rand()/(float)RAND_MAX);
}
float randf(){
	return (rand()/(float)RAND_MAX)*2.0-1.0;
}


struct VkEngine vk;



/*****************
 * SCENE & TEPOT *
 *****************/

float    *pot_vertex;
uint16_t *pot_index;

VkVertexInputBindingDescription pot_binding = {
	.binding   = 0,
	.stride    = sizeof(float) * 9, // 2 pos, 3 coor
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

VkVertexInputAttributeDescription pot_attributes[] =
{
	{
		.binding  = 0,
		.location = 0,
		.format   = VK_FORMAT_R32G32B32_SFLOAT,
		.offset   = 0,
	},
	{
		.binding  = 0,
		.location = 1,
		.format   = VK_FORMAT_R32G32B32_SFLOAT,
		.offset   = sizeof(float)*3,
	},
	{
		.binding  = 0,
		.location = 2,
		.format   = VK_FORMAT_R32G32B32_SFLOAT,
		.offset   = sizeof(float)*6,
	}
};

struct SceneUBO scene_ubo;

struct TestScene {
	int object_num;
	float *seeds;
};

struct TestScene scene = {
	.object_num = 9,
};

void init_scene(){
	int val_num = scene.object_num * 9;
	scene.seeds = malloc( sizeof(float) * val_num );
	for(int i = 0; i < val_num; i++){
		scene.seeds[i] = randf();
	}
}

void vk_load_teapot(void){

	fastObjMesh *mesh = fast_obj_read("teapot.obj");

	printf("Loaded utah teapot\n");
	printf("Groups:    %i\n", mesh->group_count);
	printf("Objects:   %i\n", mesh->object_count);
	printf("Materials: %i\n", mesh->material_count);
	printf("Faces      %i\n", mesh->face_count);
	printf("Positions  %i\n", mesh->position_count);
	printf("Indices    %i\n", mesh->index_count);
	printf("Normals    %i\n", mesh->normal_count);
	printf("UVs        %i\n", mesh->texcoord_count);

	int idx = 0;
	pot_vertex = array_create(sizeof(*pot_vertex));
	pot_index  = array_create(sizeof(*pot_index));

	for ( int i = 0; i < mesh->group_count; i++ ){
		printf("Group %i) %s\n", i, mesh->groups[i].name);

		fastObjGroup *group = &mesh->groups[i];
		
		int gidx = 0;


		for( int fi = 0; fi < group->face_count; fi++ ){
			
			uint32_t face_verts = mesh->face_vertices[ group->face_offset + fi ];

			switch (face_verts){
				case 3:
					array_push(&pot_index, (uint16_t[]){ idx+0 });
					array_push(&pot_index, (uint16_t[]){ idx+1 });
					array_push(&pot_index, (uint16_t[]){ idx+2 });
					break;
				case 4:
					array_push(&pot_index, (uint16_t[]){ idx+0 });
					array_push(&pot_index, (uint16_t[]){ idx+1 });
					array_push(&pot_index, (uint16_t[]){ idx+2 });

					array_push(&pot_index, (uint16_t[]){ idx+2 });
					array_push(&pot_index, (uint16_t[]){ idx+3 });
					array_push(&pot_index, (uint16_t[]){ idx+0 });
					break;
				default:
					printf("UNSUPPORTED POLYGON %i\n", face_verts);
					break;
			}

			
			for( int ii = 0; ii < face_verts; ii++ ){
				fastObjIndex mi = mesh->indices[group->index_offset + gidx];
					
				array_push(&pot_vertex, &mesh->positions[3 * mi.p + 0]);
				array_push(&pot_vertex, &mesh->positions[3 * mi.p + 1]);
				array_push(&pot_vertex, &mesh->positions[3 * mi.p + 2]);

				array_push(&pot_vertex, (float[]){mesh->texcoords[2 * mi.t + 0]});
				array_push(&pot_vertex, (float[]){-mesh->texcoords[2 * mi.t + 1]});

				array_push(&pot_vertex, (float[]){1.0});

				array_push(&pot_vertex, &mesh->normals[3 * mi.n + 0]);
				array_push(&pot_vertex, &mesh->normals[3 * mi.n + 1]);
				array_push(&pot_vertex, &mesh->normals[3 * mi.n + 2]);


				gidx++;
				idx++;
			}

		}

	}


	printf("Pot floats: %li\n", array_length(&pot_vertex) );
	printf("Pot indices: %i\n", idx);

	fast_obj_destroy(mesh);
}


/***************
 * ATTACH DESC *
 ***************/

void vk_create_render_pass()
{
	if(vk.error) return;

	VkAttachmentDescription color_attachment = {
		.format  = vk.swapchain_img_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference color_attachment_reference = {
		.attachment = 0,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depth_attachment = {
		.format  = vk_find_depth_format(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference depth_attachment_reference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments    = &color_attachment_reference,

		.pDepthStencilAttachment = &depth_attachment_reference,
	};

	VkSubpassDependency dep = {
		.srcSubpass     = VK_SUBPASS_EXTERNAL,
		.dstSubpass     = 0,
		.srcStageMask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		                | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask  = 0,
		.dstStageMask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		                | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};
	VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};
	VkRenderPassCreateInfo render_pass_info = {
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pAttachments    = attachments,
		.attachmentCount = LENGTH(attachments),
		.subpassCount    = 1,
		.pSubpasses      = &subpass,
		.dependencyCount = 1,
		.pDependencies   = &dep
	};
	
	VkResult ret = vkCreateRenderPass(vk.dev, &render_pass_info, NULL, &vk.render_pass);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create render pass";
		return;
	}
}

/************
 * PIPELINE *
 ************/

VkShaderModule vk_create_shader_module(enum Resource file)
{
	if(vk.error) return NULL;

	size_t spv_size;
	char  *spv = res_file(file, &spv_size);
	if(spv == NULL) {
		vk.error = "Failed to read spv file";
		return NULL;
	}

	VkShaderModuleCreateInfo create_info = {
		.sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spv_size,
		.pCode    = (void*)spv,
	};
	VkShaderModule shader_module;
	VkResult ret  = vkCreateShaderModule(vk.dev, &create_info, NULL, &shader_module);
	free(spv);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create shader module";
		return NULL;
	}
	
	return shader_module;
}

void vk_create_pipeline()
{
	if(vk.error) return;

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_TEST);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_TEST);

	if(vk.error) return;

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		{
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vert_shader,
			.pName  = "main",
			.pSpecializationInfo = NULL,
		},
		{
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = frag_shader,
			.pName  = "main",
			.pSpecializationInfo = NULL,
		}
	};

	VkPipelineVertexInputStateCreateInfo vertex_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount    = 1,
		.pVertexBindingDescriptions       = &pot_binding,

		.vertexAttributeDescriptionCount  = LENGTH(pot_attributes), 
		.pVertexAttributeDescriptions     = pot_attributes,
	};


	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
	
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width  = vk.swapchain_extent.width,
		.height = vk.swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = vk.swapchain_extent,
	};

	VkPipelineViewportStateCreateInfo viewport_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable        = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = VK_POLYGON_MODE_FILL,
		.cullMode                = VK_CULL_MODE_FRONT_BIT,
		//.cullMode                = VK_CULL_MODE_NONE,
		.frontFace               = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0,
	};
	
	VkPipelineMultisampleStateCreateInfo multisampling_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState blending_attachment_info = {
		.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT | 
			VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT | 
			VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
	};

	VkPipelineColorBlendStateCreateInfo blending_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &blending_attachment_info,
		.blendConstants[0] = 0.0f,
		.blendConstants[1] = 0.0f,
		.blendConstants[2] = 0.0f,
		.blendConstants[3] = 0.0f,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable       = VK_TRUE,
		.depthWriteEnable      = VK_TRUE,
		.depthCompareOp        = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds        = 0.0f,
		.maxDepthBounds        = 1.0f,
		.stencilTestEnable     = VK_FALSE,
		.front = {},
		.back = {},
	};

	VkDescriptorSetLayout layouts[] = { vk.descriptor_set_layout, vk.object_descriptor_layout };

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = LENGTH(layouts),
		.pSetLayouts = layouts,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
		
	};
	
	VkResult ret = vkCreatePipelineLayout(vk.dev, &pipeline_layout_info, NULL, &vk.pipeline_layout);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create pipeline layout";
		return;
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shader_stages,
		.pVertexInputState   = &vertex_info,
		.pInputAssemblyState = &input_assembly_info,
		.pViewportState      = &viewport_info,
		.pRasterizationState = &rasterizer_info,
		.pMultisampleState   = &multisampling_info,
		.pDepthStencilState  = &depth_stencil_info,
		.pColorBlendState    = &blending_info, 
		.pDynamicState       = NULL,
		.layout              = vk.pipeline_layout,
		.renderPass          = vk.render_pass,
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1,
	};
	
	ret = vkCreateGraphicsPipelines(vk.dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk.pipeline);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create graphics pipeline";
		return;
	}

    vkDestroyShaderModule(vk.dev, frag_shader, NULL);	
    vkDestroyShaderModule(vk.dev, vert_shader, NULL);	
	return;
}

/***************
 * FRAMEBUFFER *
 ***************/ 

void vk_create_framebuffers()
{
	if(vk.error) return;

	vk.framebuffers_num = vk.swapchain_img_num;
	vk.framebuffers     = malloc(sizeof(VkFramebuffer) * vk.framebuffers_num);

	for(int i = 0; i < vk.framebuffers_num; i++){
		VkImageView attachments[] = {
			vk.swapchain_img_view[i],
			vk.depth_view,
		};
		
		VkFramebufferCreateInfo fb_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = vk.render_pass,
			.attachmentCount = LENGTH(attachments),
			.pAttachments = attachments,
			.width = vk.swapchain_extent.width,
			.height = vk.swapchain_extent.height,
			.layers = 1,
		};
		
		VkResult ret = vkCreateFramebuffer(vk.dev, &fb_info, NULL, &vk.framebuffers[i]);
		if(ret != VK_SUCCESS) {
			vk.error = "Failed to create framebuffers";
			return;
		}

	}

}

/************
 * COMMANDS *
 ************/

void vk_create_cmd_pool()
{
	if(vk.error) return;
	VkResult ret; 

	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = vk.family_graphics,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		       | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT // Is this a good idea?
	};
	ret = vkCreateCommandPool(vk.dev, &pool_info, NULL, &vk.cmd_pool);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create vk.cmd_pool";
		return;
	}
}


void vk_init_frame( struct Frame *frame )
{
	if(vk.error) return;
	VkResult ret; 

	frame->object_num = 0;

	/*
	 * Command pool and buffer
	 */

	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = vk.family_graphics,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		       | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT // Is this a good idea?
	};
	ret = vkCreateCommandPool(vk.dev, &pool_info, NULL, &frame->cmd_pool);
	if(ret != VK_SUCCESS) goto failure;
	
	VkCommandBufferAllocateInfo alloc_info = {
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = frame->cmd_pool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	ret = vkAllocateCommandBuffers(vk.dev, &alloc_info, &frame->cmd_buf);
	if(ret != VK_SUCCESS) goto failure;
	
	/*
	 * Sync
	 */

	VkSemaphoreCreateInfo sema_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	ret = vkCreateSemaphore(vk.dev, &sema_info, NULL, &frame->image_available);
	if(ret != VK_SUCCESS) goto failure;
	ret = vkCreateSemaphore(vk.dev, &sema_info, NULL, &frame->render_finished);
	if(ret != VK_SUCCESS) goto failure;
	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	ret = vkCreateFence(vk.dev, &fence_info, NULL, &frame->flight);
	if(ret != VK_SUCCESS) goto failure;

	/*
	 * Uniform buffer
	 */

	vk_create_buffer_vma(
		sizeof(struct SceneUBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&frame->uniform_buffer,
		&frame->uniform_alloc
	);

	/*
	 * Descriptor sets
	 */
	
	VkDescriptorSetAllocateInfo desc_ainfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = vk.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &vk.descriptor_set_layout
	};
	ret = vkAllocateDescriptorSets(vk.dev, &desc_ainfo, &frame->descriptor_set);
	if(ret != VK_SUCCESS) goto failure;

	VkDescriptorBufferInfo uniform_buffer_info = {
		.buffer = frame->uniform_buffer,
		.offset = 0,
		.range  = sizeof(struct SceneUBO)
	};
	VkDescriptorImageInfo uniform_image_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView   = vk.texture_view,
		.sampler     = vk.texture_sampler,
	};
	VkWriteDescriptorSet desc_write[] = {
		{
			.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet             = frame->descriptor_set,
			.dstBinding         = 0,
			.dstArrayElement    = 0,
			.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount    = 1,
			.pBufferInfo        = &uniform_buffer_info,
			.pImageInfo         = NULL,
			.pTexelBufferView   = NULL,
		},
		{
			.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet             = frame->descriptor_set,
			.dstBinding         = 1,
			.dstArrayElement    = 0,
			.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount    = 1,
			.pBufferInfo        = NULL,
			.pImageInfo         = &uniform_image_info,
			.pTexelBufferView   = NULL,
		},
	};
	vkUpdateDescriptorSets(vk.dev, LENGTH(desc_write), desc_write, 0, NULL);

	// Dynamic Descriptor
	VkDescriptorSetAllocateInfo dyndesc_ainfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = vk.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &vk.object_descriptor_layout
	};
	ret = vkAllocateDescriptorSets(vk.dev, &dyndesc_ainfo, &frame->object_descriptor_dynamic);
	if(ret != VK_SUCCESS) goto failure;
	

	return;
failure:
	vk.error = "Failed to initialize frames";
}

void vk_destroy_frame( struct Frame *frame )
{
	printf("Free frame\n");
	vkDestroySemaphore(vk.dev, frame->image_available, NULL);
	vkDestroySemaphore(vk.dev, frame->render_finished, NULL);
	vkDestroyFence    (vk.dev, frame->flight,          NULL);

	vkDestroyCommandPool(vk.dev, frame->cmd_pool, NULL);

	vmaDestroyBuffer(vk.vma, frame->uniform_buffer, frame->uniform_alloc);

	if(frame->object_num){
		vmaDestroyBuffer(vk.vma, frame->object_buffer, frame->object_alloc);
	}
}

void vk_create_sync()
{
	if( vk.error ) return;
	vk.fence_image = calloc( sizeof(vk.swapchain_img_num), sizeof(VkFence) );
	return;
}

void frame_record_cmds( struct Frame *frame , int image )
{
	if(vk.error) return;
	VkResult ret;

	VkCommandBuffer cmd = frame->cmd_buf;

	vkResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
		.pInheritanceInfo = NULL,
	};
	ret = vkBeginCommandBuffer(cmd, &begin_info);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to begin recording command buffer";
		return;
	}

	VkClearValue clear_color[] = {
		{{{0.0f, 0.0f, 0.0f, 1.0f}}},
		{{{1.0,  0.0f}}}
	};

	VkRenderPassBeginInfo pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = vk.render_pass,
		.framebuffer = vk.framebuffers[image],
		.renderArea.offset = {0,0},
		.renderArea.extent = vk.swapchain_extent,
		.clearValueCount   = LENGTH(clear_color),
		.pClearValues = clear_color,
	};
	vkCmdBeginRenderPass( cmd, &pass_info, VK_SUBPASS_CONTENTS_INLINE );
	vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline );

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, &vk.vertex_buffer, offsets);
	vkCmdBindIndexBuffer(cmd, vk.index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		vk.pipeline_layout,
		0, 1,
		&frame->descriptor_set, 0, NULL
	);
	
	for( int i = 0; i < scene.object_num; i++ ){
		uint32_t dynamic_offset = sizeof(struct ObjectUBO)*i;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			vk.pipeline_layout,
			1, 1,
			&frame->object_descriptor_dynamic, 
			
			1, 
			&dynamic_offset
		);

		vkCmdDrawIndexed(cmd, array_length(&pot_index), 1, 0, 0, 0);
	}
	vkCmdEndRenderPass(cmd);

	ret = vkEndCommandBuffer(cmd);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to record command buffer";
		return;
	}
	
}



/*****************
 * STAGING BUFFER*
 *****************/


void vk_create_vertex_buffer()
{
	if(vk.error) return;

	vk_create_buffer_vma(
		array_sizeof(&pot_vertex),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&vk.staging_buffer,
		&vk.staging_alloc
	);

	if(vk.error) return;

	void *data;
	vmaMapMemory(vk.vma, vk.staging_alloc, &data);
	memcpy(data, pot_vertex, array_sizeof(&pot_vertex));
	vmaUnmapMemory(vk.vma, vk.staging_alloc);

	vk_create_buffer_vma(
		array_sizeof(&pot_vertex),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		&vk.vertex_buffer,
		&vk.vertex_alloc
	);
	if(vk.error) return;

	vk_copy_buffer(vk.staging_buffer, vk.vertex_buffer, array_sizeof(&pot_vertex));
	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);
}

void vk_create_index_buffer(){
	if(vk.error) return;

	uint32_t size = array_sizeof(&pot_index);

	vk_create_buffer_vma(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&vk.staging_buffer,
		&vk.staging_alloc
	);

	if(vk.error) return;

	void *data;
	vmaMapMemory(vk.vma, vk.staging_alloc, &data);
	memcpy(data, pot_index, size);
	vmaUnmapMemory(vk.vma, vk.staging_alloc);

	vk_create_buffer_vma(
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		&vk.index_buffer,
		&vk.index_alloc
	);
	if(vk.error) return;

	vk_copy_buffer(vk.staging_buffer, vk.index_buffer, size);
	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);
}

/************
 * UNIFORMS *
 ************/

void vk_create_scene_descriptor_layout()
{
	if(vk.error) return;

	VkDescriptorSetLayoutBinding ubo_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL,
	};

	VkDescriptorSetLayoutBinding sampler_binding = {
		.binding = 1,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImmutableSamplers = NULL,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	
	VkDescriptorSetLayoutBinding binding[] = {ubo_binding, sampler_binding};

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = LENGTH(binding),
		.pBindings = binding,
	};

	VkResult ret;
	ret = vkCreateDescriptorSetLayout(vk.dev, &layout_info, NULL, &vk.descriptor_set_layout);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create descriptor set layout";
		return;
	}
}


void vk_create_object_descriptor_layout()
{
	if(vk.error) return;

	VkDescriptorSetLayoutBinding ubo_binding = {
		.binding = 0, 
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL,
	};
	
	VkDescriptorSetLayoutBinding binding[] = {ubo_binding};

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = LENGTH(binding),
		.pBindings = binding,
	};

	VkResult ret;
	ret = vkCreateDescriptorSetLayout(vk.dev, &layout_info, NULL, &vk.object_descriptor_layout);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create object descriptor layout";
		return;
	}
}

void vk_update_object_buffer(struct Frame *frame)
{
	if(vk.error) return;
	double now   = glfwGetTime();

	// Clear previous frame
	if(frame->object_num){
		vmaDestroyBuffer(vk.vma, frame->object_buffer, frame->object_alloc);
	}
	frame->object_num = 0;
	
	if( scene.object_num == 0 ) return;
	frame->object_num = scene.object_num;

	// Allocate 
	struct ObjectUBO *object_buffer = malloc( sizeof(struct ObjectUBO) * scene.object_num );

	vk_create_buffer_vma(
		sizeof(struct ObjectUBO) * scene.object_num,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&frame->object_buffer,
		&frame->object_alloc
	);

	// Fill
	for( int i = 0; i < scene.object_num; i++ ){
		mat4 model; 
		glm_mat4_identity(model);

		float *s = scene.seeds+(i*9);

		float t = now*0.3;
		float x = sin(t+i*0.7)*5.0;
		float y = cos(t+i*0.7)*5.0;
		float z = s[8]+1.5;

		glm_translate_x(model, x);
		glm_translate_y(model, y);
		glm_translate_z(model, z);
		
		float axis [3] = {s[0]*M_PI, s[1]*M_PI, s[2]*M_PI};
		float axis2[3] = {s[3]*M_PI, s[4]*M_PI, s[5]*M_PI};
		glm_vec3_normalize(axis);
		
		glm_rotate(model, glm_rad(s[6]*360.0), axis);
		glm_rotate(model, glm_rad(s[7]*now*100.0), axis2);

		float scale = 0.10;
		glm_scale(model, (float[]){scale, scale, scale});

		glm_translate_z(model, -5.0);

		memcpy(object_buffer[i].model, model, sizeof(model));
	}


	VkDescriptorBufferInfo object_buffer_info = {
		.buffer = frame->object_buffer,
		.offset = 0,
		.range  = sizeof(struct ObjectUBO)
	};

	VkWriteDescriptorSet dyndesc_write[] = {
		{
			.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet             = frame->object_descriptor_dynamic,
			.dstBinding         = 0, 
			.dstArrayElement    = 0,
			.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount    = 1,
			.pBufferInfo        = &object_buffer_info,
			.pImageInfo         = NULL,
			.pTexelBufferView   = NULL,
		},
	};
	vkUpdateDescriptorSets(vk.dev, LENGTH(dyndesc_write), dyndesc_write, 0, NULL);



	void *data;
	vmaMapMemory(vk.vma, frame->object_alloc, &data);
	memcpy(data, object_buffer, sizeof(struct ObjectUBO) * scene.object_num);
	vmaUnmapMemory(vk.vma, frame->object_alloc);
	
	free(object_buffer);
}

void vk_update_uniform_buffer(struct Frame *frame)
{
	if(vk.error) return;
	static double last = 0;

	double now   = glfwGetTime();

	glm_mat4_identity(scene_ubo.view);
	glm_mat4_identity(scene_ubo.proj);

	
	glm_perspective(
		glm_rad(45.0f), 
		vk.swapchain_extent.width / (float) vk.swapchain_extent.height,
		0.1f, 1000.0f,
		scene_ubo.proj
	);

	glm_lookat( 
		(vec4){0.0f, 10.0f, 10.0f},
		(vec4){0.0f, 0.0f, 0.0f},
		(vec4){0.0f, 0.0f, 1.0f},
		scene_ubo.view
	);

	scene_ubo.proj[1][1] *= -1; // ????

	last = now;

	void *data;
	vmaMapMemory(vk.vma, frame->uniform_alloc, &data);
	memcpy(data, &scene_ubo, sizeof(scene_ubo));
	vmaUnmapMemory(vk.vma, frame->uniform_alloc);

}


void vk_create_descriptor_pool()
{
	if(vk.error) return;
	VkDescriptorPoolSize pool_sizes[] = {
		{
			.type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 32,
		},

		{
			.type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 32,
		},
		{
			.type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 32,
		},
	};

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = LENGTH(pool_sizes),
		.pPoolSizes = pool_sizes,
		.maxSets = 64,
		.flags  = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
	};

	VkResult ret;
	ret = vkCreateDescriptorPool(vk.dev, &pool_info, NULL, &vk.descriptor_pool);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create descriptor pool";
		return;
	}


}
/********
 * DRAW *
 ********/

void vk_recreate_swapchain()
{
	if(vk.error) return;
	
	double now = glfwGetTime();
	if(vk.last_resize + 0.25 > now ) return;

	int width = 0, height = 0;
	glfwGetFramebufferSize(vk.window, &width, &height);
	while(width==0 || height == 0){
		glfwGetFramebufferSize(vk.window, &width, &height);
		glfwWaitEvents();
	}


	vkDeviceWaitIdle(vk.dev);

	vk_destroy_swapchain();
	vk_create_swapchain();
	vk_create_image_views();

	vk_create_render_pass();
	vk_create_pipeline();
	vk_create_depth_resources();
	vk_create_framebuffers();

	vk.framebuffer_resize = false;
	vk.last_resize = now;
}



void draw_frame()
{
	if( vk.error ) return;
	VkResult ret;

	struct Frame *frame = &vk.frames[vk.current_frame];

	vkWaitForFences(vk.dev, 1, &frame->flight, VK_TRUE, UINT64_MAX);
    uint32_t image_index;
    ret = vkAcquireNextImageKHR(vk.dev, vk.swapchain, 
			UINT64_MAX, 
			frame->image_available,
			VK_NULL_HANDLE, 
			&image_index);

	if(ret == VK_ERROR_OUT_OF_DATE_KHR ){
		vk_recreate_swapchain();
		return;
	}else if(ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR){
		vk.error = "Unhandled image acquire error";
		return;
	}
	
	if(vk.fence_image[image_index] != VK_NULL_HANDLE){
		vkWaitForFences(vk.dev, 1, &vk.fence_image[image_index], VK_TRUE, UINT64_MAX);
	}
	vk.fence_image[image_index] = frame->flight;

	VkSemaphore          wait_semas[]  = {frame->image_available};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore          sig_semas[]   = {frame->render_finished};

	vk_update_object_buffer(frame);
	if(vk.error) return;

	vk_update_uniform_buffer(frame);
	if(vk.error) return;
	
	frame_record_cmds( frame, image_index );

	VkSubmitInfo submit_info = {
		.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = wait_semas,
		.pWaitDstStageMask  = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers    = &frame->cmd_buf,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores  = sig_semas, 
			
	};
	
	vkResetFences  (vk.dev, 1, &frame->flight);
	ret = vkQueueSubmit(vk.graphics_queue, 1, &submit_info, frame->flight);
	if(ret != VK_SUCCESS){
		vk.error = "Submit failure";
		return;
	}

	VkSwapchainKHR swap_chains[] = {vk.swapchain};
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = sig_semas,
		.swapchainCount     = 1,
		.pSwapchains        = swap_chains,
		.pImageIndices      = &image_index,
		.pResults           = NULL,
	};
	ret = vkQueuePresentKHR(vk.present_queue, &present_info);

	if(ret == VK_ERROR_OUT_OF_DATE_KHR 
	|| ret == VK_SUBOPTIMAL_KHR 
	|| vk.framebuffer_resize){
		vk_recreate_swapchain();
	}else if(ret != VK_SUCCESS){
		vk.error = "Presentation failure";
		return;
	}
	vk.current_frame = (vk.current_frame+1) % VK_MAX_FRAMES_IN_FLIGHT;
}


/***********
 * TEXTURE *
 ***********/

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

	if( old_layout == VK_IMAGE_LAYOUT_UNDEFINED 
	&&  new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		src_stage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	else 
	if( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
	&&  new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		src_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else 
	{
		vk.error = "Unsupported layout transition";
		return;
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

void vk_create_texture(void)
{
	if(vk.error) return;
	int texw, texh, texch;
	stbi_uc *pixels = stbi_load("texture.png", &texw, &texh, &texch, STBI_rgb_alpha);
	
	if(!pixels){
		vk.error = "Failed to load texture (file missing?)";
		return;
	}

	VkDeviceSize   image_size = texw * texh * 4;
	
	vk_create_buffer_vma(
		image_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, 
		&vk.staging_buffer,
		&vk.staging_alloc
	);

	void *data;

	vmaMapMemory(vk.vma, vk.staging_alloc, &data);
	memcpy(data, pixels, image_size);
	vmaUnmapMemory(vk.vma, vk.staging_alloc);
	
	stbi_image_free(pixels);
	
	vk_create_image_vma(
		texw, texh,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		&vk.texture_image,
		&vk.texture_alloc
	);

	vk_transition_image_layout(vk.texture_image, 
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	vk_copy_buffer_to_image(vk.staging_buffer, vk.texture_image, texw, texh);

	vk_transition_image_layout(vk.texture_image,
		VK_FORMAT_R8G8B8A8_SRGB, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);
}


void vk_create_texture_view()
{
	if(vk.error) return;
	
	VkImageViewCreateInfo create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = vk.texture_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format   = VK_FORMAT_R8G8B8A8_SRGB,

		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,

		.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel   = 0,
		.subresourceRange.levelCount     = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount     = 1,
	};

	VkResult ret = vkCreateImageView(vk.dev, &create_info, NULL, &vk.texture_view);

	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create image view for texture";
		return;
	};
}

void vk_create_texture_sampler()
{
	if(vk.error) return;

	VkSamplerCreateInfo sampler_info = {
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter               = VK_FILTER_LINEAR,
		.minFilter               = VK_FILTER_LINEAR,
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable        = VK_TRUE,
		.maxAnisotropy           = vk.dev_properties.limits.maxSamplerAnisotropy,
		.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
		.compareEnable           = VK_FALSE,
		.compareOp               = VK_COMPARE_OP_ALWAYS,
		.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias              = 0.0f,
		.minLod                  = 0.0f,
		.maxLod                  = 0.0f,
	};

	VkResult ret = vkCreateSampler(vk.dev, &sampler_info, NULL, &vk.texture_sampler);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create sampler";
		return;
	}

}

/*********
 * DEPTH *
 *********/

VkFormat vk_find_supported_format( 
	VkFormat *formats,
	size_t    formats_num,
	VkImageTiling tiling, 
	VkFormatFeatureFlags features 
){
	
	for(int i = 0; i < formats_num; i++){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vk.dev_physical, formats[i], &props);

		if (tiling == VK_IMAGE_TILING_LINEAR 
		&& (props.linearTilingFeatures & features) == features)
			return formats[i];

		if (tiling == VK_IMAGE_TILING_OPTIMAL 
		&& (props.optimalTilingFeatures & features) == features)
			return formats[i];

	}
	vk.error = "No supported formats";
	return 0;
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

bool vk_has_stencil(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT 
		|| format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void vk_create_depth_resources()
{
	if(vk.error) return;

	VkFormat depth_format = vk_find_depth_format();
	if(vk.error) return;

	vk_create_image_vma(
		vk.swapchain_extent.width,
		vk.swapchain_extent.height,
		depth_format,
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
		VMA_MEMORY_USAGE_GPU_ONLY,
		&vk.depth_image,
		&vk.depth_alloc
	);

	// Create image view
	VkImageViewCreateInfo create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = vk.depth_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format   = depth_format,

		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,

		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};
	VkResult ret = vkCreateImageView(vk.dev, &create_info, NULL, &vk.depth_view);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create depth image view";
		return;
	};
}

/*******
 * VMA *
 *******/

void vk_create_allocator()
{
	if(vk.error) return;

	VmaAllocatorCreateInfo vma_info = {
		.vulkanApiVersion = VK_API_VERSION_1_2,
		.physicalDevice   = vk.dev_physical,
		.device           = vk.dev,
		.instance         = vk.instance,
	};
	VkResult ret = vmaCreateAllocator(&vma_info, &vk.vma);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create VMA";
		return;
	};
}

/********
 * MAIN *
 ********/

static void vk_resize_callback(GLFWwindow *window, int w, int h)
{
	vk.framebuffer_resize = true;
}

int ctx_init(void)
{
	memset( &vk, 0, sizeof(struct VkEngine) );
	vk._verbose = true;

	vk_load_teapot();
	init_scene();

	if(!glfwInit()){
		printf("GLFW Init failure");
		return 1;
	}

	if (glfwVulkanSupported())
		printf("Vulkan supported\n");
	else{
		printf("Vulkan not supported\n");
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);
	vk.window = glfwCreateWindow(800,600,"Vulkan vk.window",NULL,NULL);
	glfwSetFramebufferSizeCallback(vk.window, vk_resize_callback);


	vk_instance_ext_get_avbl();
	vk_validation_get_avbl();
	vk_validation_add("VK_LAYER_KHRONOS_validation");
	vk_create_instance(); 

	if(vk.error){
		printf("%s\n", vk.error);
		return -1;
	}

	glfwCreateWindowSurface(vk.instance, vk.window, NULL, &vk.surface);

	vk_device_ext_add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	vk_create_device();

	vkGetPhysicalDeviceProperties(vk.dev_physical, &vk.dev_properties);

	vk_create_allocator();


	vk_create_swapchain();
	vk_create_image_views();
	vk_create_texture_sampler();
	vk_create_render_pass();

	vk_create_scene_descriptor_layout();
	vk_create_object_descriptor_layout();

	vk_create_pipeline();

	vk_create_cmd_pool();

	vk_create_depth_resources();
	vk_create_framebuffers();
	vk_create_texture();
	vk_create_texture_view();
	vk_create_vertex_buffer();
	vk_create_index_buffer();
	vk_create_descriptor_pool();
	vk_create_sync();

	for(int i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; i++){
		vk_init_frame( &vk.frames[i] );
	}
	
	int frames = 0;
	double last_title = glfwGetTime();
	double last =       glfwGetTime();

	printf("MinAligment %li\n", vk.dev_properties.limits.minUniformBufferOffsetAlignment); // TODO!!!

	while (!glfwWindowShouldClose(vk.window) && !vk.error) {

		double now = glfwGetTime();
		double delta = now - last;
		last = now;

		if( last_title + 1.0 < now ){
			static char title[128];
			snprintf(title, 128, "Vulkan [%i FPS] [%0.2f ms]\n", frames, (delta*1000.0));
			glfwSetWindowTitle(vk.window, title);
			last_title += 1.0;
			frames = 0;
		}


		glfwPollEvents();
		draw_frame();
		frames++;
		usleep(1000*3); // Temp framerate limiter
	}

	if(vk.dev) vkDeviceWaitIdle(vk.dev);

	if(vk.error){
		printf("Vulkan error: %s\n", vk.error);
		printf("cerrnostr: %s\n", strerror(errno));
		return 1; // Unclean exit!!
	}

	vk_destroy_swapchain();

	vkDestroySampler(vk.dev, vk.texture_sampler, NULL);
	
	vkDestroyImageView(vk.dev, vk.texture_view, NULL);
	vmaDestroyImage( vk.vma, vk.texture_image, vk.texture_alloc);

	vkDestroyDescriptorSetLayout(vk.dev, vk.descriptor_set_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, vk.object_descriptor_layout, NULL);

	vmaDestroyBuffer(vk.vma, vk.vertex_buffer, vk.vertex_alloc);

	vmaDestroyBuffer(vk.vma, vk.index_buffer, vk.index_alloc);
	
	for(int i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; i++){
		vk_destroy_frame( &vk.frames[i] );
	}

	vkDestroyDescriptorPool(vk.dev, vk.descriptor_pool, NULL);
	vkDestroyCommandPool(vk.dev, vk.cmd_pool, NULL);

	printf("Free khr\n");
	vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);

	printf("Free vma\n");
	vmaDestroyAllocator(vk.vma);
	printf("Free dev\n");
	vkDestroyDevice(vk.dev, NULL);
	printf("Free ins\n");
	vkDestroyInstance(vk.instance, NULL);

	printf("Free win\n");
	glfwDestroyWindow(vk.window);
	printf("Terminate\n");
	glfwTerminate();

	return 0;
}
