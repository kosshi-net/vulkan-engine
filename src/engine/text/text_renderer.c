#include "text/text_renderer.h"
#include "text/text_geometry.h"
#include "text/text_engine.h"

#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"

#include "common.h"
#include "res/res.h"
#include "event/event.h"
#include "handle/handle.h"

extern struct VkEngine vk;

static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct TextRenderer, 1);

struct TextRenderer *text_renderer_get_struct(TextEngine handle)
{
	return handle_dereference(&alloc, handle);
}

static VkVertexInputBindingDescription text_binding = {
	.binding   = 0,
	.stride    = sizeof(struct TextVertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

static VkVertexInputAttributeDescription text_attributes[] = {
	{
		.binding  = 0,
		.location = 0,
		.format   = VK_FORMAT_R32G32_SFLOAT,
		.offset   = offsetof(struct TextVertex, pos),
	},
	{
		.binding  = 0,
		.location = 1,
		.format   = VK_FORMAT_R32G32_SFLOAT,
		.offset   = offsetof(struct TextVertex, uv),
	},
	{
		.binding  = 0,
		.location = 2,
		.format   = VK_FORMAT_R8G8B8A8_UNORM,
		.offset   = offsetof(struct TextVertex, color),
	},
};

void vk_text_create_pipeline(struct TextRenderer *restrict this)
{
	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_TEXT);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_TEXT);


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
		.pVertexBindingDescriptions       = &text_binding,

		.vertexAttributeDescriptionCount  = LENGTH(text_attributes), 
		.pVertexAttributeDescriptions     = text_attributes,
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
		//.cullMode                = VK_CULL_MODE_FRONT_BIT,
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
		.depthTestEnable       = VK_FALSE,
		.depthWriteEnable      = VK_FALSE,
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
		.size = sizeof(struct TextUniform)
		}
	};

	VkDescriptorSetLayout layouts[] = { 
		this->descriptor_layout, 
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = LENGTH(layouts),
		.pSetLayouts = layouts,
		.pushConstantRangeCount = LENGTH(push_constants),
		.pPushConstantRanges = push_constants,
	};
	
	VkResult ret = vkCreatePipelineLayout(vk.dev, 
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

void vk_text_create_descriptor_layout(struct TextRenderer *restrict this)
{
	VkDescriptorSetLayoutBinding binding[] = {
		{
			.binding = 0,
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

	VkResult ret;
	ret = vkCreateDescriptorSetLayout(vk.dev, 
		&layout_info, NULL, &this->descriptor_layout
	);

	if (ret != VK_SUCCESS) engine_crash("vkCreateDescriptorSetLayout");
}

void vk_text_update_texture(struct TextRenderer *restrict this)
{
	struct TextEngine *restrict engine = text_engine_get_struct(this->engine);

	int32_t  texw   = engine->atlas.w;
	int32_t  texh   = engine->atlas.h;
	uint8_t *pixels = engine->atlas.bitmap;
	VkDeviceSize   image_size = texw * texh * 1;

	vk_create_buffer_vma(
		image_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, 
		&vk.staging_buffer,
		&vk.staging_alloc
	);

	vk_transition_image_layout(this->texture_image, 
		VK_FORMAT_R8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	void *data;

	vmaMapMemory(vk.vma, vk.staging_alloc, &data);
	memcpy(data, pixels, image_size);
	vmaUnmapMemory(vk.vma, vk.staging_alloc);

	vk_copy_buffer_to_image(vk.staging_buffer, this->texture_image, texw, texh);
	vk_transition_image_layout(this->texture_image,
		VK_FORMAT_R8_SRGB, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);

	engine->atlas.dirty = false;
}

void vk_text_texture(struct TextRenderer *restrict this)
{
	struct TextEngine *restrict engine = text_engine_get_struct(this->engine);

	int32_t texw = engine->atlas.w;
	int32_t texh = engine->atlas.h;

	vk_create_image_vma(
		texw, texh,
		VK_FORMAT_R8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		&this->texture_image,
		&this->texture_alloc
	);
	
	vk_text_update_texture(this);

	/* view */
	
	VkImageViewCreateInfo create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = this->texture_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format   = VK_FORMAT_R8_SRGB,

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

	VkResult ret = vkCreateImageView(vk.dev, 
		&create_info, NULL, &this->texture_view
	);

	if(ret != VK_SUCCESS) engine_crash("vkCreateImageView failed");
}



void vk_text_create_fbdeps(struct TextRenderer *restrict this)
{
	vk_text_create_descriptor_layout(this);
	vk_text_create_pipeline(this);
}

void vk_text_destroy_fbdeps(struct TextRenderer *restrict this)
{
	vkDestroyPipeline(      vk.dev, this->pipeline, NULL);
	vkDestroyPipelineLayout(vk.dev, this->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, this->descriptor_layout, NULL);
}

/************
 * EXTERNAL *
 ************/

void text_renderer_destroy_callback(TextRenderer id, void*p)
{
	text_renderer_destroy(id);
}

TextRenderer text_renderer_destroy(TextRenderer handle)
{
	struct TextRenderer *restrict this = text_renderer_get_struct(handle);

	vkDestroyImageView(vk.dev, this->texture_view, NULL);
	vmaDestroyImage   (vk.vma, this->texture_image, this->texture_alloc);
	vmaDestroyBuffer  (vk.vma, this->index_buffer,  this->index_alloc);

	vk_text_destroy_fbdeps(this);

	return 0;
}

uint16_t *create_index_buffer(size_t max_glyphs, size_t *size)
{
	static const uint16_t quad_index[] = { 
		0, 1, 2,
		1, 2, 3
	};

	*size = max_glyphs * sizeof(quad_index);
	uint16_t *buffer = malloc(*size);

	for (ufast32_t i = 0; i < max_glyphs; i++) {
		fast32_t offset = LENGTH(quad_index) * i;
		for (ufast32_t j = 0; j < LENGTH(quad_index); j++)
			buffer[offset+j] = 4*i + quad_index[j];
	}

	return buffer;
}

TextRenderer text_renderer_create(TextEngine engine_handle)
{
	TextRenderer                  handle = handle_allocate(&alloc);
	struct TextRenderer *restrict this   = text_renderer_get_struct(handle);
	this->engine = engine_handle;

	vk_text_texture(this);

	size_t index_buf_size;
	void  *index_buf = create_index_buffer(
		32*1024, &index_buf_size
	);

	vk_upload_buffer( 
		&this->index_buffer, &this->index_alloc,  
		index_buf, index_buf_size, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	vk_text_create_fbdeps(this);

	event_bind(EVENT_RENDERERS_DESTROY, text_renderer_destroy_callback, handle);

	return handle;
}

void text_renderer_draw(
	TextRenderer handle,
	TextGeometry geom_handle,
	struct Frame *frame)
{
	struct TextRenderer *restrict this   = text_renderer_get_struct(handle);
	struct TextGeometry *restrict geom   = text_geometry_get_struct(geom_handle);
	struct TextEngine   *restrict engine = text_engine_get_struct(this->engine);

	uint32_t f = (geom->type == TEXT_GEOMETRY_STATIC) ? 0 : frame->vk->id;

	struct TextUniform ubo;

	glm_ortho(
		0.0, vk.swapchain_extent.width,
		0.0, vk.swapchain_extent.height,
		-1.0, 1.0,
		ubo.ortho
	);


	if (geom->type == TEXT_GEOMETRY_DYNAMIC) {
		text_geometry_upload(geom_handle, frame);
	}

	if (engine->atlas.dirty) {
		log_debug("Uploading atlas!");
		vk_text_update_texture(this);
	}

	VkCommandBuffer cmd = frame->vk->cmd_buf;

	vkCmdPushConstants(cmd, 
		this->pipeline_layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0, sizeof(struct TextUniform),
		&ubo
	);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &geom->frame[f].vertex_buffer, (VkDeviceSize[]){0});
	vkCmdBindIndexBuffer(cmd, this->index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		this->pipeline_layout,
		0, 1,
		&geom->frame[f].descriptor_set, 0, NULL
	);

	uint32_t glyphs = MIN(geom->glyph_count, geom->glyph_max);

	vkCmdDrawIndexed(cmd, glyphs * TEXT_INDICES_PER_GLYPH, 1,0,0,0);
}
