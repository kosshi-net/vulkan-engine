#include "wireframe_renderer.h"

#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"

#include "common.h"
#include "res/res.h"
#include "event/event.h"
#include "handle/handle.h"

extern struct VkEngine vk;

/* Structures */

struct WireframeUniform {
	mat4 proj;
	mat4 view;
};

struct WireframeVertex {
	float   pos[3];
	uint8_t color[4];
};

struct WireframeRenderer {

	VkPipelineLayout               pipeline_layout;
	VkPipeline                     pipeline;

	struct WireframeVertex         *vertex_buffer;
	uint32_t                        vertex_count;
	uint32_t                        vertex_max;
	uint32_t                        vertex_truncated;

	struct {
		VkBuffer         vertex_buffer;
		VmaAllocation    vertex_alloc;
		void            *vertex_mapping;

	} frame[VK_FRAMES];



};

static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct WireframeRenderer, 1);

static void vk_create(struct WireframeRenderer *restrict this)
{
	VkResult ret;

	static VkVertexInputBindingDescription vertex_binding[] = {
		{
			.binding   = 0,
			.stride    = sizeof(struct WireframeVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
	};

	static VkVertexInputAttributeDescription vertex_attributes[] = {
		{
			.binding  = 0,
			.location = 0,
			.format   = VK_FORMAT_R32G32B32_SFLOAT,
			.offset   = offsetof(struct WireframeVertex, pos),
		},
		{
			.binding  = 0,
			.location = 1,
			.format   = VK_FORMAT_R8G8B8A8_SRGB,
			.offset   = offsetof(struct WireframeVertex, color),
		}
	};

	/* Pipeline */

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_WIREFRAME);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_WIREFRAME);

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
		.vertexBindingDescriptionCount    = LENGTH(vertex_binding),
		.pVertexBindingDescriptions       = vertex_binding,

		.vertexAttributeDescriptionCount  = LENGTH(vertex_attributes), 
		.pVertexAttributeDescriptions     = vertex_attributes,
	};


	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
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
			.size = sizeof(struct WireframeUniform)
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


void wireframe_renderer_destroy_callback(WireframeRenderer handle, void*p)
{
	wireframe_renderer_destroy(&handle);
}

void wireframe_renderer_destroy(WireframeRenderer *handle)
{
	struct WireframeRenderer *restrict this = handle_deref(&alloc, *handle);

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

	handle_free(&alloc, handle);
}

WireframeRenderer wireframe_renderer_create(void)
{
	WireframeRenderer                  handle = handle_alloc(&alloc);
	struct WireframeRenderer *restrict this   = handle_deref(&alloc, handle);


	log_debug("Uniform size: %li", sizeof(struct WireframeUniform));

	vk_create(this);

	this->vertex_max = 1<<20;

	/* Frames */

	VkDeviceSize vksize = this->vertex_max * sizeof(struct WireframeVertex);

	log_debug("%.2f MB of VRAM allocated", (vksize*VK_FRAMES)/1024.0/1024.0  );

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

	event_bind(EVENT_RENDERERS_DESTROY, wireframe_renderer_destroy_callback, handle);

	return handle;
}

void wireframe_renderer_draw(
	WireframeRenderer handle,
	struct Frame *frame)
{
	struct WireframeRenderer *restrict this = handle_deref(&alloc, handle);

	uint32_t f = frame->vk->id;

	if (this->vertex_truncated) {
		log_warn("Truncated vertices: %i", this->vertex_truncated);
		this->vertex_truncated = 0;
	}

	/* Upload geometry */

	size_t bytes = this->vertex_count * sizeof(struct WireframeVertex);
	memcpy(
		this->frame[f].vertex_mapping, 
		this->vertex_buffer, 
		bytes
	);
	vmaFlushAllocation(vk.vma, this->frame[f].vertex_alloc, 0, bytes);


	/* Push constants */
	static struct WireframeUniform ubo;

	glm_mat4_copy(frame->camera.view,       ubo.view);
	glm_mat4_copy(frame->camera.projection, ubo.proj);

	VkCommandBuffer cmd = frame->vk->cmd_buf;

	vkCmdPushConstants(cmd, 
		this->pipeline_layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0, sizeof(struct WireframeUniform),
		&ubo
	);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 1, 
		&this->frame[f].vertex_buffer, 
		(VkDeviceSize[]){0}
	);

	vkCmdDraw(cmd, this->vertex_count, 1,0,0);

	this->vertex_count = 0;
}

void wireframe_point(
		WireframeRenderer handle,
		float p[3],
		uint32_t color)
{
	struct WireframeRenderer *restrict this = handle_deref(&alloc, handle);

	if (this->vertex_count >= this->vertex_max) {
		this->vertex_truncated++;
		return;
	};

	struct WireframeVertex *restrict vert = &this->vertex_buffer[this->vertex_count++];
	
	memcpy(vert->pos, p, sizeof(vert->pos));

	vert->color[0] = (color >> 24) & 0xFF;
	vert->color[1] = (color >> 16) & 0xFF;
	vert->color[2] = (color >> 8 ) & 0xFF;
	vert->color[3] = (color >> 0 ) & 0xFF;
}

void wireframe_line(
		WireframeRenderer handle,
		float p1[3], 
		float p2[3], 
		uint32_t color)
{
	wireframe_point(handle, p1, color);
	wireframe_point(handle, p2, color);
}

void wireframe_triangle(
		WireframeRenderer handle,
		float p1[3], 
		float p2[3], 
		float p3[3], 
		uint32_t color)
{
	wireframe_point(handle, p1, color);
	wireframe_point(handle, p2, color);
	wireframe_point(handle, p2, color);
	wireframe_point(handle, p3, color);
	wireframe_point(handle, p3, color);
	wireframe_point(handle, p1, color);
}

