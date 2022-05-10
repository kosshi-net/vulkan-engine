#include "gfx/text/vk_text.h"

#include "common.h"
#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"
#include "res.h"
#include "gfx/text/text.h"

#define TEXT_FRAME_MAX_GLYPHS (1024*4)
#define TEXT_VERTICES_PER_GLYPH 4
#define TEXT_INDICES_PER_GLYPH 6

extern struct VkEngine vk;

static struct TextContext *_txtctx = NULL; // TODO

static VkVertexInputBindingDescription text_binding = {
	.binding   = 0,
	.stride    = sizeof(float) * 6, 
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

static VkVertexInputAttributeDescription text_attributes[] =
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
};

void vk_text_create_pipeline()
{
	if(vk.error) return;

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_TEXT);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_TEXT);

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
		.front = {},
		.back = {},
	};

	VkDescriptorSetLayout layouts[] = { 
		vk.text.descriptor_layout, 
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = LENGTH(layouts),
		.pSetLayouts = layouts,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};
	
	VkResult ret = vkCreatePipelineLayout(vk.dev, 
			&pipeline_layout_info, NULL, &vk.text.pipeline_layout
	);
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
		.layout              = vk.text.pipeline_layout,
		.renderPass          = vk.renderpass,
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1,
	};
	
	ret = vkCreateGraphicsPipelines(vk.dev, 
			VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk.text.pipeline
		);
	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create graphics pipeline";
		return;
	}

    vkDestroyShaderModule(vk.dev, frag_shader, NULL);	
    vkDestroyShaderModule(vk.dev, vert_shader, NULL);	
	return;
}

void vk_text_create_descriptor_layout()
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
		&layout_info, NULL, &vk.text.descriptor_layout
	);
	if(ret != VK_SUCCESS){
		vk.error = "Failed to create object descriptor layout";
		return;
	}
}

void vk_text_update_texture(struct TextContext *ctx)
{
	int32_t  texw   = ctx->atlas.w;
	int32_t  texh   = ctx->atlas.h;
	uint8_t *pixels = ctx->atlas.bitmap;
	VkDeviceSize   image_size = texw * texh * 1;

	vk_create_buffer_vma(
		image_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, 
		&vk.staging_buffer,
		&vk.staging_alloc
	);



	vk_transition_image_layout(vk.text.texture_image, 
		VK_FORMAT_R8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	void *data;

	vmaMapMemory(vk.vma, vk.staging_alloc, &data);
	memcpy(data, pixels, image_size);
	vmaUnmapMemory(vk.vma, vk.staging_alloc);

	vk_copy_buffer_to_image(vk.staging_buffer, vk.text.texture_image, texw, texh);
	vk_transition_image_layout(vk.text.texture_image,
		VK_FORMAT_R8_SRGB, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);


	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);

	ctx->atlas.dirty = false;
}

void vk_text_texture(struct TextContext *ctx)
{
	if(vk.error) return;

	int32_t  texw   = ctx->atlas.w;
	int32_t  texh   = ctx->atlas.h;

	vk_create_image_vma(
		texw, texh,
		VK_FORMAT_R8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		&vk.text.texture_image,
		&vk.text.texture_alloc
	);
	
	vk_text_update_texture(ctx);

	/* view */
	
	VkImageViewCreateInfo create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = vk.text.texture_image,
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
		&create_info, NULL, &vk.text.texture_view
	);

	if(ret != VK_SUCCESS) {
		vk.error = "Failed to create image view for texture";
		return;
	};
}

void vk_text_create_fbdeps()
{
	vk_text_create_descriptor_layout();
	vk_text_create_pipeline();
}

void vk_text_create(void)
{
	vk_text_texture(_txtctx);

	size_t index_buf_size;
	void  *index_buf = txt_create_shared_index_buffer(
		TEXT_FRAME_MAX_GLYPHS, &index_buf_size
	);

	vk_upload_buffer( 
		&vk.text.index_buffer, &vk.text.index_alloc,  
		index_buf, index_buf_size, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	vk_text_create_fbdeps();
}

void vk_text_frame_destroy(struct Frame *frame)
{

	vmaUnmapMemory(vk.vma, frame->text.vertex_alloc);
	vmaDestroyBuffer(vk.vma, 
		frame->text.vertex_buffer, 
		frame->text.vertex_alloc
	);


}

void vk_text_frame_create(struct Frame *frame)
{
	if (vk.error) return; 
	VkResult ret; 

	size_t max_vertices = TEXT_FRAME_MAX_GLYPHS * 4;
	size_t max_indices  = TEXT_FRAME_MAX_GLYPHS * 6;
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

	ret = vmaCreateBuffer(vk.vma, 
		&buffer_info, &alloc_info, 
		&frame->text.vertex_buffer, 
		&frame->text.vertex_alloc, 
		NULL
	);

    if (ret != VK_SUCCESS) {
		vk.error = "Failed to allocate (vma)";
		return;
    }

	size = max_indices * sizeof(uint16_t);
	buffer_info = (VkBufferCreateInfo){
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

    if (ret != VK_SUCCESS) {
		vk.error = "Failed to allocate (vma)";
		return;
    }

	vmaMapMemory(vk.vma, frame->text.vertex_alloc, &frame->text.vertex_mapping);
}

void vk_text_destroy_fbdeps()
{
	vkDestroyPipeline(      vk.dev, vk.text.pipeline, NULL);
	vkDestroyPipelineLayout(vk.dev, vk.text.pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, vk.text.descriptor_layout, NULL);
}

void vk_text_destroy(void)
{
	vkDestroyImageView(vk.dev, vk.text.texture_view, NULL);
	vmaDestroyImage( vk.vma, vk.text.texture_image, vk.text.texture_alloc);
	vmaDestroyBuffer(vk.vma, vk.text.index_buffer,  vk.text.index_alloc);
}

void vk_text_frame_update(struct Frame *frame)
{
	frame->text.index_count = _txtctx->glyph_count * 6;

	memcpy(frame->text.vertex_mapping, 
		_txtctx->vertex_buffer, 
		array_sizeof(_txtctx->vertex_buffer)
	);

	vmaFlushAllocation(vk.vma, frame->text.vertex_alloc, 0, 
		array_sizeof(_txtctx->vertex_buffer)
	);

	if (_txtctx->atlas.dirty) {
		printf("Uploading atlas!\n");
		vk_text_update_texture(_txtctx);
	}

}


void vk_text_update_uniform_buffer(struct Frame *frame)
{
	if(vk.error) return;
	
	struct TextUBO ubo;

	glm_ortho(
		0.0, vk.swapchain_extent.width,
		0.0, vk.swapchain_extent.height,
		
		-1.0, 1.0,
		ubo.ortho
	);

	void *data;
	vmaMapMemory(vk.vma, frame->text.uniform_alloc, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vmaUnmapMemory(vk.vma, frame->text.uniform_alloc);

	vk_text_frame_update(frame);
}



void vk_text_add_ctx(struct TextContext *ctx)
{
	_txtctx = ctx;

}
