#include "widget/widget_renderer.h"

#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"

#include "common.h"
#include "res/res.h"
#include "event/event.h"
#include "handle/handle.h"

extern struct VkEngine vk;

static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct WidgetRenderer, 1);

static VkVertexInputBindingDescription text_binding = {
	.binding   = 0,
	.stride    = sizeof(struct WidgetVertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

static VkVertexInputAttributeDescription text_attributes[] = {
	{
		.binding  = 0,
		.location = 0,
		.format   = VK_FORMAT_R32G32_SFLOAT,
		.offset   = offsetof(struct WidgetVertex, pos),
	},
	{
		.binding  = 0,
		.location = 1,
		.format   = VK_FORMAT_R8G8B8A8_UNORM,
		.offset   = offsetof(struct WidgetVertex, color),
	},
	{
		.binding  = 0,
		.location = 2,
		.format   = VK_FORMAT_R16_UNORM,
		.offset   = offsetof(struct WidgetVertex, depth),
	},
};

static void vk_create(struct WidgetRenderer *restrict this)
{
	VkResult ret;

	/* Pipeline */

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_WIDGET);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_WIDGET);

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
		.size = sizeof(struct WidgetUniform)
		}
	};


	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
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


void widget_renderer_destroy_callback(WidgetRenderer handle, void*p)
{
	widget_renderer_destroy(&handle);
}

void widget_renderer_destroy(WidgetRenderer *handle)
{
	struct WidgetRenderer *restrict this = handle_deref(&alloc, *handle);

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

	free(this->vertex_buffer);

	handle_free(&alloc, handle);
}

static uint16_t *create_index_buffer(size_t max_glyphs, size_t *size)
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

WidgetRenderer widget_renderer_create(void)
{
	WidgetRenderer                  handle = handle_alloc(&alloc);
	struct WidgetRenderer *restrict this   = handle_deref(&alloc, handle);

	this->quad_max = 1024;


	size_t index_buf_size;
	void  *index_buf = create_index_buffer(
		this->quad_max, &index_buf_size
	);

	vk_upload_buffer( 
		&this->index_buffer, &this->index_alloc,  
		index_buf, index_buf_size, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);


	VkResult ret; 
	size_t max_vertices = this->quad_max * 4;

	this->vertex_buffer = malloc(sizeof(struct WidgetVertex) * max_vertices);

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = max_vertices * sizeof(struct WidgetVertex),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
	};

	for (ufast32_t i = 0; i < VK_FRAMES; i++) {
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

	vk_create(this);

	event_bind(EVENT_RENDERERS_DESTROY, widget_renderer_destroy_callback, handle);

	return handle;
}


void widget_renderer_clear(WidgetRenderer handle)
{
	struct WidgetRenderer *restrict this = handle_deref(&alloc, handle);

	this->quad_count = 0;

}

static void widget_geometry_upload(
	WidgetRenderer handle, 
	struct Frame *restrict frame) 
{
	struct WidgetRenderer *restrict this = handle_deref(&alloc, handle);
	
	uint32_t f = frame->vk->id;

	size_t quads = this->quad_count;
	size_t bytes = quads * 6 * sizeof(struct WidgetVertex);
	memcpy(
		this->frame[f].vertex_mapping, 
		this->vertex_buffer, 
		bytes
	);

	vmaFlushAllocation(vk.vma, this->frame[f].vertex_alloc, 0, bytes);
}

void widget_renderer_draw(
	WidgetRenderer handle,
	struct Frame *frame)
{
	struct WidgetRenderer *restrict this = handle_deref(&alloc, handle);

	uint32_t f = frame->vk->id;

	struct WidgetUniform ubo;

	glm_ortho(
		0.0, vk.swapchain_extent.width,
		0.0, vk.swapchain_extent.height,
		-1.0, 1.0,
		ubo.ortho
	);

	widget_geometry_upload(handle, frame);

	VkCommandBuffer cmd = frame->vk->cmd_buf;

	vkCmdPushConstants(cmd, 
		this->pipeline_layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0, sizeof(struct WidgetUniform),
		&ubo
	);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, &this->frame[f].vertex_buffer, (VkDeviceSize[]){0});
	vkCmdBindIndexBuffer(cmd, this->index_buffer, 0, VK_INDEX_TYPE_UINT16);


	uint32_t quads = MIN(this->quad_count, this->quad_max);

	vkCmdDrawIndexed(cmd, quads * 6, 1,0,0,0);
}


void  widget_renderer_quad(WidgetRenderer handle, 
	int32_t x, int32_t y, int32_t w, int32_t h,
	uint16_t depth,
	uint32_t color)
{
	struct WidgetRenderer *restrict this = handle_deref(&alloc, handle);

	static const int32_t quad_vertex[4][3] = {
		{0, 0, 0}, 
		{0, 1, 0}, 
		{1, 0, 0}, 
		{1, 1, 0}, 
	};

	struct WidgetVertex *vert = &this->vertex_buffer[this->quad_count * 4];
	this->quad_count++;

	for (int i = 0; i < 4; i++) {
		vert[i].pos[0] = (x + w * quad_vertex[i][0]);
		vert[i].pos[1] = (y + h * quad_vertex[i][1]);

		vert[i].depth = depth;

		vert[i].color[0] = color >> 24;
		vert[i].color[1] = color >> 16;
		vert[i].color[2] = color >> 8;
		vert[i].color[3] = color >> 0;
	}
}
