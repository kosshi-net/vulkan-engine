#include "gfx/text/vk_text.h"

#include "common.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"
#include "res.h"
#include "gfx/text/text.h"
#include "event/event.h"

#define TEXT_VERTICES_PER_GLYPH 4
#define TEXT_INDICES_PER_GLYPH 6

extern struct VkEngine vk;

static struct VkTextContext vktxtctx[4];


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

void vk_text_create_pipeline(struct VkTextContext *restrict this)
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
		/*.front = {},
		.back = {},*/
	};

	VkDescriptorSetLayout layouts[] = { 
		this->descriptor_layout, 
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = LENGTH(layouts),
		.pSetLayouts = layouts,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
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

void vk_text_create_descriptor_layout(struct VkTextContext *restrict this)
{
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
	
	VkDescriptorSetLayoutBinding binding[] = {
		ubo_binding, sampler_binding
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

void vk_text_update_texture(struct VkTextContext *restrict this)
{
	int32_t  texw   = this->ctx->atlas.w;
	int32_t  texh   = this->ctx->atlas.h;
	uint8_t *pixels = this->ctx->atlas.bitmap;
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

	this->ctx->atlas.dirty = false;
}

void vk_text_texture(struct VkTextContext *restrict this)
{
	int32_t  texw   = this->ctx->atlas.w;
	int32_t  texh   = this->ctx->atlas.h;

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



void vk_text_create_fbdeps(struct VkTextContext *restrict this)
{
	vk_text_create_descriptor_layout(this);
	vk_text_create_pipeline(this);
}

void vk_text_destroy_fbdeps(struct VkTextContext *restrict this)
{
	vkDestroyPipeline(      vk.dev, this->pipeline, NULL);
	vkDestroyPipelineLayout(vk.dev, this->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, this->descriptor_layout, NULL);
}

/************
 * EXTERNAL *
 ************/

void gfx_text_renderer_destroy(TextRenderer id)
{
	struct VkTextContext *restrict this = &vktxtctx[id];

	for (int i = 0; i < VK_FRAMES; i++) {
		vmaUnmapMemory(vk.vma, this->frame[i].vertex_alloc);
		vmaDestroyBuffer(vk.vma, 
			this->frame[i].vertex_buffer, 
			this->frame[i].vertex_alloc
		);

		vmaDestroyBuffer(vk.vma, 
			this->frame[i].uniform_buffer, this->frame[i].uniform_alloc
		);
	}

	vkDestroyImageView(vk.dev, this->texture_view, NULL);
	vmaDestroyImage   (vk.vma, this->texture_image, this->texture_alloc);
	vmaDestroyBuffer  (vk.vma, this->index_buffer,  this->index_alloc);

	vk_text_destroy_fbdeps(this);
}

uint32_t gfx_text_renderer_create(
	struct TextContext *txtctx, 
	uint32_t max_glyphs
){
	VkResult ret; 
	uint32_t id;
	for (id = 0; id < LENGTH(vktxtctx); id++) {
		if (vktxtctx[id].ctx == NULL) 
			goto free_found;
	}
	engine_crash("Too many text renderers");
free_found:;
	struct VkTextContext *this = &vktxtctx[id];
	this->ctx = txtctx;
	this->max_glyphs = max_glyphs;
	vk_text_texture(this);

	size_t index_buf_size;
	void  *index_buf = txt_create_shared_index_buffer(
		max_glyphs, &index_buf_size
	);

	vk_upload_buffer( 
		&this->index_buffer, &this->index_alloc,  
		index_buf, index_buf_size, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	vk_text_create_fbdeps(this);

	/*****************
	 * Create frames *
	 *****************/

	size_t max_vertices = this->max_glyphs * 4;
	size_t size = max_vertices * sizeof(float) * 6;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
	};

	for (ufast32_t i = 0; i < VK_FRAMES; i++) {
		vk_create_buffer_vma(
			sizeof(struct TextUBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			&this->frame[i].uniform_buffer,
			&this->frame[i].uniform_alloc
		);

		VkDescriptorSetAllocateInfo desc_ainfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool     = vk.descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts        = &this->descriptor_layout
		};
		ret = vkAllocateDescriptorSets(vk.dev, 
			&desc_ainfo, &this->frame[i].descriptor_set
		);
		if(ret != VK_SUCCESS) engine_crash("vkAllocateDescriptorSets failed");

		VkDescriptorBufferInfo uniform_buffer_info = {
			.buffer = this->frame[i].uniform_buffer,
			.offset = 0,
			.range  = sizeof(struct TextUBO)
		};
		VkDescriptorImageInfo uniform_image_info = {
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.imageView   = this->texture_view,
			.sampler     = vk.texture_sampler,
		};
		VkWriteDescriptorSet desc_write[] = {
			{
				.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet             = this->frame[i].descriptor_set,
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
				.dstSet             = this->frame[i].descriptor_set,
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

		ret = vmaCreateBuffer(vk.vma, 
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

	return id;
}


void gfx_text_draw(struct Frame *frame, TextRenderer id)
{
	struct VkTextContext *restrict this = &vktxtctx[id];
	uint32_t f = frame->vk->id;

	struct TextUBO ubo;

	glm_ortho(
		0.0, vk.swapchain_extent.width,
		0.0, vk.swapchain_extent.height,
		-1.0, 1.0,
		ubo.ortho
	);

	void *data;
	vmaMapMemory(vk.vma, this->frame[f].uniform_alloc, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vmaUnmapMemory(vk.vma, this->frame[f].uniform_alloc);

	size_t glyphs = this->ctx->glyph_count;

	if (glyphs > this->max_glyphs) {
		log_error("Max glyphs exceeded (%li > %li)", 
			glyphs, this->max_glyphs
		);
		glyphs = this->max_glyphs;
	}

	size_t bytes = glyphs * TEXT_VERTICES_PER_GLYPH * sizeof(struct TextVertex);
	memcpy(
		this->frame[f].vertex_mapping, 
		this->ctx->vertex_buffer, 
		bytes
	);

	vmaFlushAllocation(vk.vma, this->frame[f].vertex_alloc, 0, bytes);

	if (this->ctx->atlas.dirty) {
		log_debug("Uploading atlas!");
		vk_text_update_texture(this);
	}

	VkCommandBuffer cmd = frame->vk->cmd_buf;

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &this->frame[f].vertex_buffer, (VkDeviceSize[]){0});
	vkCmdBindIndexBuffer(cmd, this->index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		this->pipeline_layout,
		0, 1,
		&this->frame[f].descriptor_set, 0, NULL
	);

	vkCmdDrawIndexed(cmd, glyphs * TEXT_INDICES_PER_GLYPH, 1,0,0,0);
}


