#include "softbody_renderer.h"
#include "softbody.h"

#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"

#include "common.h"
#include "res/res.h"
#include "event/event.h"
#include "handle/handle.h"

#include "mem/mem.h"
#include "mem/arr.h"

#define DEPTH_FORMAT VK_FORMAT_D32_SFLOAT
#define SHADOW_FILTER VK_FILTER_NEAREST
extern struct VkEngine vk;

struct SoftbodyUniform {
	mat4 proj;
	mat4 view;
};

struct LightUniform {
	mat4 matrix;
	vec3 dir;
};

struct SoftbodyVertex {
	float pos[3];
	float normal[3];
};

struct SoftbodyRenderer {


	VkPipelineLayout               pipeline_layout;
	VkPipeline                     pipeline;

	VkBuffer                       index_buffer;
	VmaAllocation                  index_alloc;
	uint32_t                       index_count;

	struct SoftbodyVertex         *vertex_buffer;

	struct SoftbodyRendererFrame {
		VkBuffer         vertex_buffer;
		VmaAllocation    vertex_alloc;
		void            *vertex_mapping;
		
		VkDescriptorSet  scene_descriptor;
	} frame[VK_FRAMES];

	VkBuffer          uniform_buffer;
	VmaAllocation     uniform_alloc;

	/* Only for depth sampler */
	VkDescriptorSetLayout scene_layout;

	VkSampler         shadow_sampler;


	VkImage           shadow_image;
	VmaAllocation     shadow_alloc;
	VkImageView       shadow_view;

	VkRenderPass      shadow_renderpass;

	VkPipelineLayout  shadow_pipeline_layout;
	VkPipeline        shadow_pipeline;

	VkFramebuffer    *shadow_framebuffer;


};

static VkVertexInputBindingDescription vertex_binding = {
	.binding   = 0,
	.stride    = sizeof(struct SoftbodyVertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

static VkVertexInputAttributeDescription vertex_attributes[] = {
	{
		.binding  = 0,
		.location = 0,
		.format   = VK_FORMAT_R32G32B32_SFLOAT,
		.offset   = offsetof(struct SoftbodyVertex, pos),
	},
	{
		.binding  = 0,
		.location = 1,
		.format   = VK_FORMAT_R32G32B32_SFLOAT,
		.offset   = offsetof(struct SoftbodyVertex, normal),
	}
};

#define SHADOW_RESOLUTION 4096

static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct SoftbodyRenderer, 1);



/***********
 * Shadows *
 ***********/

static void vk_create_shadow_descriptor(struct SoftbodyRenderer *this)
{
	VkResult ret;

	for (size_t i = 0; i < VK_FRAMES; i++) {
		struct SoftbodyRendererFrame *frame = &this->frame[i];

		
		VkDescriptorSetAllocateInfo desc_ainfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool     = vk.descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts        = &this->scene_layout
		};
		ret = vkAllocateDescriptorSets(vk.dev, &desc_ainfo, &frame->scene_descriptor);
		if(ret != VK_SUCCESS) engine_crash("vkAllocateDescriptorSets failed");

		VkWriteDescriptorSet desc_write[] = {
			{
				.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet             = frame->scene_descriptor,
				.dstBinding         = 0,
				.dstArrayElement    = 0,
				.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount    = 1,
				.pBufferInfo        = (VkDescriptorBufferInfo[]){{
					.buffer          = this->uniform_buffer,
					.offset          = 0,
					.range           = sizeof(struct LightUniform),
				}},
				.pImageInfo         = NULL,
				.pTexelBufferView   = NULL,
			},
			{
				.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet             = frame->scene_descriptor,
				.dstBinding         = 1,
				.dstArrayElement    = 0,
				.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount    = 1,
				.pBufferInfo        = NULL,
				.pImageInfo         = (VkDescriptorImageInfo[]){{
					.imageLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
					.imageView       = this->shadow_view,
					.sampler         = this->shadow_sampler,
				}},
				.pTexelBufferView   = NULL,
			},
		};
		vkUpdateDescriptorSets(vk.dev, LENGTH(desc_write), desc_write, 0, NULL);
	}
}

static void vk_create_shadow_layout(struct SoftbodyRenderer *this)
{
	VkResult ret;

	VkDescriptorSetLayoutBinding binding[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},
		{
			.binding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImmutableSamplers = NULL,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		}
	};
	
	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = LENGTH(binding),
		.pBindings = binding,
	};

	ret = vkCreateDescriptorSetLayout(vk.dev, 
		&layout_info, NULL, &this->scene_layout
	);

	if (ret != VK_SUCCESS) engine_crash("vkCreateDescriptorSetLayout failed");

}

static void vk_create_shadow_sampler(struct SoftbodyRenderer *this)
{
	VkSamplerCreateInfo sampler_info = {
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter               = SHADOW_FILTER,
		.minFilter               = SHADOW_FILTER,
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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

	VkResult ret = vkCreateSampler(vk.dev, &sampler_info, NULL, &this->shadow_sampler);

	if(ret != VK_SUCCESS) engine_crash("vkCreateSampler failed");
}

static void vk_create_shadow_framebuffer(struct SoftbodyRenderer *this)
{
	this->shadow_framebuffer  = mem_malloc(sizeof(VkFramebuffer) * vk.framebuffers_num);

	for (size_t i = 0; i < vk.framebuffers_num; i++) {
		VkImageView attachments[] = {
			this->shadow_view,
		};

		VkFramebufferCreateInfo fb_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass      = this->shadow_renderpass,
			.attachmentCount = LENGTH(attachments),
			.pAttachments    = attachments,
			.width           = SHADOW_RESOLUTION,
			.height          = SHADOW_RESOLUTION,
			.layers = 1,
		};
		
		VkResult ret = vkCreateFramebuffer(vk.dev, &fb_info, NULL, 
			&this->shadow_framebuffer[i]
		);
		if(ret != VK_SUCCESS) engine_crash("vkCreateFramebuffer failed");
	}
}

static void vk_create_shadow_resources(struct SoftbodyRenderer *this)
{

	vk_create_image_vma(
		SHADOW_RESOLUTION,
		SHADOW_RESOLUTION,
		DEPTH_FORMAT,
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VMA_MEMORY_USAGE_GPU_ONLY,
		&this->shadow_image,
		&this->shadow_alloc
	);

	VkImageViewCreateInfo create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = this->shadow_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format   = DEPTH_FORMAT,

		.components = {
			.r = VK_COMPONENT_SWIZZLE_R,
			.g = VK_COMPONENT_SWIZZLE_G,
			.b = VK_COMPONENT_SWIZZLE_B,
			.a = VK_COMPONENT_SWIZZLE_A,
		},

		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

	VkResult ret = vkCreateImageView(vk.dev, &create_info, NULL, &this->shadow_view);
	if (ret != VK_SUCCESS) engine_crash("vkCreateImageView failed");
}

static void vk_create_shadow_renderpass(struct SoftbodyRenderer *this)
{

	VkAttachmentDescription depth_attachment = {
		.format  = DEPTH_FORMAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
	};
	VkAttachmentReference depth_attachment_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 0,
		.pColorAttachments    = NULL,
		.pDepthStencilAttachment = &depth_attachment_reference,
	};

	VkSubpassDependency dep[] = {
		{
			.srcSubpass     = VK_SUBPASS_EXTERNAL,
			.dstSubpass     = 0,

			.srcStageMask   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,

			.srcAccessMask  = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,

			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		},
		{
			.srcSubpass     = 0,
			.dstSubpass     = VK_SUBPASS_EXTERNAL,

			.srcStageMask   = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,

			.srcAccessMask  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask  = VK_ACCESS_SHADER_READ_BIT,

			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
		}
	};
	VkAttachmentDescription attachments[] = {depth_attachment};
	VkRenderPassCreateInfo render_pass_info = {
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pAttachments    = attachments,
		.attachmentCount = LENGTH(attachments),
		.subpassCount    = 1,
		.pSubpasses      = &subpass,
		.dependencyCount = LENGTH(dep),
		.pDependencies   = dep
	};
	
	VkResult ret = vkCreateRenderPass(vk.dev, &render_pass_info, NULL, 
			&this->shadow_renderpass);

	if(ret != VK_SUCCESS) engine_crash("vkCreateRenderPass failed");
}



static void vk_create_shadow_pipeline(struct SoftbodyRenderer *this)
{
	VkResult ret;

	/* Pipeline */

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_SOFTBODY_SHADOW);

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		{
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vert_shader,
			.pName  = "main",
			.pSpecializationInfo = NULL,
		}
	};

	VkPipelineVertexInputStateCreateInfo vertex_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount    = 1,
		.pVertexBindingDescriptions       = &vertex_binding,

		.vertexAttributeDescriptionCount  = LENGTH(vertex_attributes), 
		.pVertexAttributeDescriptions     = vertex_attributes,
	};


	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
	
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width  = SHADOW_RESOLUTION,
		.height = SHADOW_RESOLUTION,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		//.extent = vk.swapchain_extent,
		.extent = {
			.width = SHADOW_RESOLUTION,
			.height = SHADOW_RESOLUTION,
		}
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
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = VK_POLYGON_MODE_FILL,
		.cullMode                = VK_CULL_MODE_NONE,
		.frontFace               = VK_FRONT_FACE_CLOCKWISE,
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

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable       = VK_TRUE,
		.depthWriteEnable      = VK_TRUE,
		.depthCompareOp        = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds        = 0.0f,
		.maxDepthBounds        = 1.0f,
		.stencilTestEnable     = VK_FALSE,
	};

	VkPushConstantRange push_constants[] = {
		{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(struct SoftbodyUniform)
		}
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts    = &this->scene_layout,
		.pushConstantRangeCount = LENGTH(push_constants),
		.pPushConstantRanges = push_constants,
	};
	
	ret = vkCreatePipelineLayout(vk.dev, 
			&pipeline_layout_info, NULL, &this->shadow_pipeline_layout
	);
	if (ret != VK_SUCCESS) engine_crash("vkCreatePipelineLayout failed");

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount          = LENGTH(shader_stages),
		.pStages             = shader_stages,
		.pVertexInputState   = &vertex_info,
		.pInputAssemblyState = &input_assembly_info,
		.pViewportState      = &viewport_info,
		.pRasterizationState = &rasterizer_info,
		.pMultisampleState   = &multisampling_info,
		.pDepthStencilState  = &depth_stencil_info,
		.pColorBlendState    = NULL, 
		.pDynamicState       = NULL,
		.layout              = this->shadow_pipeline_layout,
		.renderPass          = this->shadow_renderpass,
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1,
	};
	
	ret = vkCreateGraphicsPipelines(vk.dev, 
		VK_NULL_HANDLE, 1, &pipeline_info, NULL, &this->shadow_pipeline
	);

	if (ret != VK_SUCCESS) engine_crash("vkCreateGraphicsPipelines failed");

    vkDestroyShaderModule(vk.dev, vert_shader, NULL);
}



static void vk_create_shadow(struct SoftbodyRenderer *this)
{
	vk_create_shadow_resources(this);
	vk_create_shadow_renderpass(this);

	vk_create_shadow_sampler(this);
	vk_create_shadow_layout(this);
	vk_create_shadow_descriptor(this);

	vk_create_shadow_framebuffer(this);

	vk_create_shadow_pipeline(this);
}

/**************
 * END SHADOW *
 **************/

/************
 * UNIFORMW *
 ************/

static void vk_create_uniform_buffer(struct SoftbodyRenderer *this)
{
	vk_create_buffer_vma(
		sizeof(struct LightUniform),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&this->uniform_buffer,
		&this->uniform_alloc
	);

	/* Light direction is largely static, write here */


	struct LightUniform ubo = {
		.dir = { 0.0, 5.0, 5.0 }
	};

	const float size = 3;


	glm_ortho(
		-size, size, 
		-size, size,
		0.01, 30.0,
		ubo.matrix
	);
	ubo.matrix[1][1] *= -1; 

	mat4 light_view;

	glm_lookat(ubo.dir, 
		(float[]){0.0f, 0.0f, 0.0f},
		(float[]){0.0f, 1.0f, 0.0f},
		light_view
	);

	vec3 offset = {0.0, 0.0, 3.0};
	glm_translate(light_view, offset);

	glm_mul(ubo.matrix, light_view, ubo.matrix);

	glm_normalize(ubo.dir);

	//glm_translate(ubo.matrix, (float[]){ -0.5f, -0.5f, -5.0f });
	//glm_rotate_x(ubo.matrix, 45, ubo.matrix);
	
	if(0)
	glm_lookat(
		(float[]){0.0f, 2.0f, -2.0f},
		(float[]){0.0f, 0.0f, 0.0f},
		(float[]){0.0f, 1.0f, 0.0f},
		ubo.matrix
	);

	void *data;
	vmaMapMemory(vk.vma, this->uniform_alloc, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vmaUnmapMemory(vk.vma, this->uniform_alloc);


}



static void vk_create(struct SoftbodyRenderer *this)
{
	VkResult ret;

	/* Pipeline */

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_SOFTBODY);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_SOFTBODY);

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
		.pVertexBindingDescriptions       = &vertex_binding,

		.vertexAttributeDescriptionCount  = LENGTH(vertex_attributes), 
		.pVertexAttributeDescriptions     = vertex_attributes,
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

	VkDynamicState dynamic_state[] = {
		VK_DYNAMIC_STATE_VIEWPORT, 
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.flags = 0,
		.dynamicStateCount = LENGTH(dynamic_state),
		.pDynamicStates    = dynamic_state 
	};

	VkPipelineRasterizationStateCreateInfo rasterizer_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = VK_POLYGON_MODE_FILL,
		.cullMode                = VK_CULL_MODE_NONE,
		.frontFace               = VK_FRONT_FACE_CLOCKWISE,
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
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
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
	};

	VkPushConstantRange push_constants[] = {
		{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(struct SoftbodyUniform)
		}
	};


	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &this->scene_layout,
		.pushConstantRangeCount = LENGTH(push_constants),
		.pPushConstantRanges = push_constants,
	};
	
	ret = vkCreatePipelineLayout(vk.dev, 
			&pipeline_layout_info, NULL, &this->pipeline_layout
	);
	if (ret != VK_SUCCESS) engine_crash("vkCreatePipelineLayout failed");

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
		.pDynamicState       = &dynamic_info,
		.layout              = this->pipeline_layout,
		.renderPass          = vk.renderpass,
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1,
	};
	
	ret = vkCreateGraphicsPipelines(vk.dev, 
		VK_NULL_HANDLE, 1, &pipeline_info, NULL, &this->pipeline
	);

	if (ret != VK_SUCCESS) engine_crash("vkCreateGraphicsPipelines failed");

    vkDestroyShaderModule(vk.dev, frag_shader, NULL);	
    vkDestroyShaderModule(vk.dev, vert_shader, NULL);
}


void softbody_renderer_destroy_callback(SoftbodyRenderer handle, void*p)
{
	softbody_renderer_destroy(&handle);
}

void softbody_renderer_destroy(SoftbodyRenderer *handle)
{
	struct SoftbodyRenderer *restrict this = handle_deref(&alloc, *handle);

	vmaDestroyBuffer  (vk.vma, this->index_buffer,  this->index_alloc);
	vkDestroyPipeline (vk.dev, this->pipeline, NULL);
	vkDestroyPipelineLayout(vk.dev, this->pipeline_layout, NULL);
	
	for (ufast32_t i = 0; i < VK_FRAMES; i++) {
		vmaUnmapMemory(vk.vma, this->frame[i].vertex_alloc);
		vmaDestroyBuffer(vk.vma, 
			this->frame[i].vertex_buffer, 
			this->frame[i].vertex_alloc
		);

	}

	mem_free(this->vertex_buffer);

	for (size_t i = 0; i < vk.framebuffers_num; i++) {
		vkDestroyFramebuffer(vk.dev, this->shadow_framebuffer[i], NULL);
	}
	mem_free(this->shadow_framebuffer);

	vmaDestroyImage(vk.vma, this->shadow_image, this->shadow_alloc);
	vmaDestroyBuffer(vk.vma, this->uniform_buffer, this->uniform_alloc);
	
	vkDestroyImageView(vk.dev, this->shadow_view, NULL);
	vkDestroySampler(vk.dev, this->shadow_sampler, NULL);

	vkDestroyRenderPass(vk.dev, this->shadow_renderpass, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, this->scene_layout, NULL);

	vkDestroyPipelineLayout(vk.dev, this->shadow_pipeline_layout, NULL);
	vkDestroyPipeline(vk.dev, this->shadow_pipeline, NULL);


	handle_free(&alloc, handle);
}

/**********
 * CREATE *
 **********/

SoftbodyRenderer softbody_renderer_create(void)
{
	SoftbodyRenderer                  handle = handle_alloc(&alloc);
	struct SoftbodyRenderer *restrict this   = handle_deref(&alloc, handle);


	log_debug("Uniform size: %li", sizeof(struct SoftbodyUniform));

	vk_create_uniform_buffer(this);
	vk_create_shadow(this);
	vk_create(this);

	event_bind(EVENT_RENDERERS_DESTROY, softbody_renderer_destroy_callback, handle);

	return handle;
}


void softbody_geometry_update(
		SoftbodyRenderer handle,
		struct SBMesh *restrict mesh, 
		struct Frame *restrict frame)
{
	struct SoftbodyRenderer *restrict this = handle_deref(&alloc, handle);

	
	for (size_t i = 0; i < mesh->vertex_count; i++) {
		memcpy(
			this->vertex_buffer[i].pos,
			mesh->vertex[i],
			sizeof(vec3)
		);

		memcpy(
			this->vertex_buffer[i].normal,
			mesh->normal[i],
			sizeof(vec3)

		);
	}

	uint32_t f = frame->vk->id;

	size_t bytes = mesh->vertex_count * sizeof(struct SoftbodyVertex);

	memcpy(
		this->frame[f].vertex_mapping, 
		this->vertex_buffer, 
		bytes
	);

	vmaFlushAllocation(vk.vma, this->frame[f].vertex_alloc, 0, bytes);
}

void softbody_geometry_prepare(
		SoftbodyRenderer handle, 
		struct SBMesh *restrict mesh)
{
	struct SoftbodyRenderer *restrict this = handle_deref(&alloc, handle);

	/* Index */

	vk_upload_buffer( 
		&this->index_buffer, &this->index_alloc,  
		mesh->index, arr_size(mesh->index), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	this->index_count = mesh->index_count;

	/* Vertex */

	VkDeviceSize vksize = mesh->vertex_count * sizeof(struct SoftbodyVertex);

	this->vertex_buffer = mem_malloc(vksize);


	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = vksize,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
	};

	for (size_t i = 0; i < VK_FRAMES; i++) {
		VkResult ret = vmaCreateBuffer(vk.vma, 
			&buffer_info, &alloc_info, 
			&this->frame[i].vertex_buffer, 
			&this->frame[i].vertex_alloc, 
			NULL
		);

		if(ret != VK_SUCCESS) engine_crash("vmaCreateBuffer failed");

		vmaMapMemory(vk.vma, 
			 this->frame[i].vertex_alloc, 
			&this->frame[i].vertex_mapping
		);
	}
}


void softbody_renderer_draw_shadowmap(
	SoftbodyRenderer handle,
	struct Frame *frame)
{
	struct SoftbodyRenderer *restrict this = handle_deref(&alloc, handle);


	VkCommandBuffer cmd = frame->vk->cmd_buf;
	uint32_t f = frame->vk->id;

	VkClearValue clear_color[] = 
	{
		{
			.depthStencil = {1.0f, 0.0f},
		}
	};

	/*
	VkClearValue clear_color[] = {

		{{{1.0,  0.0f, 0.0f, 1.0f}}},
		{{{1.0,  0.0f}}}
	};*/

	VkExtent2D extent = {
		.width  = SHADOW_RESOLUTION,
		.height = SHADOW_RESOLUTION,
	};

	VkRenderPassBeginInfo pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass  = this->shadow_renderpass,
		.framebuffer = this->shadow_framebuffer[ frame->vk->image_index ],
		//.framebuffer = this->shadow_framebuffer[ 0],
		.renderArea.offset = {0,0},
		.renderArea.extent = extent,
		.clearValueCount   = LENGTH(clear_color),
		.pClearValues      = clear_color,
	};
	vkCmdBeginRenderPass(cmd, &pass_info, VK_SUBPASS_CONTENTS_INLINE);

	static struct SoftbodyUniform ubo;
	glm_mat4_copy(frame->camera.view,       ubo.view);
	glm_mat4_copy(frame->camera.projection, ubo.proj);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		this->pipeline_layout,
		0, 1,
		&this->frame[f].scene_descriptor, 0, NULL
	);

	vkCmdPushConstants(cmd, 
		this->shadow_pipeline_layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0, sizeof(struct SoftbodyUniform),
		&ubo
	);


	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->shadow_pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &this->frame[f].vertex_buffer, (VkDeviceSize[]){0});
	vkCmdBindIndexBuffer(cmd, this->index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(cmd, this->index_count, 1,0,0,0);

	vkCmdEndRenderPass(cmd);
}

static void print_mat(mat4 m) {
	log_info("Matrix:");
	for (size_t i = 0; i < 4; i++) {
		log_info("%f %f %f %f",
			m[i][0], 
			m[i][1], 
			m[i][2], 
			m[i][3]
		);
	}

}

void softbody_renderer_draw(
	SoftbodyRenderer handle,
	struct Frame *frame)
{
	struct SoftbodyRenderer *restrict this = handle_deref(&alloc, handle);

	uint32_t f = frame->vk->id;

	static struct SoftbodyUniform ubo;

	glm_mat4_copy(frame->camera.view,       ubo.view);
	glm_mat4_copy(frame->camera.projection, ubo.proj);

	
	VkCommandBuffer cmd = frame->vk->cmd_buf;

	vkCmdPushConstants(cmd, 
		this->pipeline_layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0, sizeof(struct SoftbodyUniform),
		&ubo
	);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		this->pipeline_layout,
		0, 1,
		&this->frame[f].scene_descriptor, 0, NULL
	);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &this->frame[f].vertex_buffer, (VkDeviceSize[]){0});
	vkCmdBindIndexBuffer(cmd, this->index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(cmd, this->index_count, 1,0,0,0);
}
