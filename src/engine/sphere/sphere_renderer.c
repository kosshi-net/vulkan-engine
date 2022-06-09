#include "sphere_renderer.h"

#include "gfx/gfx.h"
#include "gfx/gfx_types.h"
#include "gfx/vk_util.h"

#include "common.h"
#include "res/res.h"
#include "event/event.h"
#include "handle/handle.h"

#include "icosphere.h"

extern struct VkEngine vk;

/* Struct definitions */

struct SceneUniform {
	mat4 proj;
	mat4 view;
};

struct ObjectUniform {
	mat4   model;
	float  color[4];
} __attribute__ ((aligned ((256))));

struct SphereVertex {
	float   pos[3];
};

#define SPHERE_LODS 3

struct SphereRenderer {

	VkPipelineLayout       pipeline_layout;
	VkPipeline             pipeline;


	VkBuffer               vertex_buffer;
	VmaAllocation          vertex_alloc;

	/* Uniform */
	VkDescriptorSetLayout  scene_layout;

	struct {
		uint32_t         index_count;
		VkBuffer         index_buffer;
		VmaAllocation    index_alloc;
	} lod[SPHERE_LODS];

	struct SphereInstance {
		float pos[3];
		float radius;
		float color[4];
	}       *sphere;
	uint32_t sphere_count;
	uint32_t sphere_max;

	struct SphereRendererFrame{
		
		VkBuffer         instance_buffer;
		VmaAllocation    instance_alloc;
		void            *instance_mapping;

		VkDescriptorSet  scene_uniform;
		VkBuffer         scene_buffer;
		VmaAllocation    scene_alloc;
		void            *scene_mapping;


	} frame[VK_FRAMES];
};



static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct SphereRenderer, 1);

static void vk_create_frame(
	struct SphereRenderer      *this,
	struct SphereRendererFrame *frame)
{
	VkResult ret;

	/* Scene */

	VkDescriptorSetAllocateInfo desc_alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = vk.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &this->scene_layout
	};


	ret = vkAllocateDescriptorSets(vk.dev, &desc_alloc_info, 
		&frame->scene_uniform
	);
	if(ret != VK_SUCCESS) engine_crash("vkAllocateDescriptorSets failed");

	vk_create_buffer_vma(
		sizeof(struct SceneUniform),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&frame->scene_buffer,
		&frame->scene_alloc
	);

	vmaMapMemory(vk.vma, 
		 frame->scene_alloc, 
		&frame->scene_mapping
	);

	VkDescriptorBufferInfo uniform_buffer_info = {
		.buffer = frame->scene_buffer,
		.offset = 0,
		.range  = sizeof(struct SceneUniform)
	};
	VkWriteDescriptorSet desc_write[] = {
		{
			.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet             = frame->scene_uniform,
			.dstBinding         = 0,
			.dstArrayElement    = 0,
			.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount    = 1,
			.pBufferInfo        = &uniform_buffer_info,
			.pImageInfo         = NULL,
			.pTexelBufferView   = NULL,
		}
	};
	vkUpdateDescriptorSets(vk.dev, LENGTH(desc_write), desc_write, 0, NULL);
}

static void vk_create(struct SphereRenderer *restrict this)
{
	VkResult ret;

	/**************
	 * Descriptor *
	 **************/

	{
		VkDescriptorSetLayoutBinding uniform_binding[] = {
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			},
		};

		VkDescriptorSetLayoutCreateInfo layout_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = LENGTH(uniform_binding),
			.pBindings = uniform_binding
		};

		ret = vkCreateDescriptorSetLayout(vk.dev, 
			&layout_info, NULL, &this->scene_layout
		);
		if (ret != VK_SUCCESS) 
			engine_crash("vkCreateDescriptorSetLayout failed");

	}



	for (size_t i = 0; i < VK_FRAMES; i++) {
		vk_create_frame(this, &this->frame[i]);
	}

	/* Vertex desc */

	static VkVertexInputBindingDescription vertex_binding[] = {
		{
			.binding   = 0,
			.stride    = sizeof(struct SphereVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		},

		{
			.binding   = 1,
			.stride    = sizeof(struct SphereInstance), 
			.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
		}
	};

	static VkVertexInputAttributeDescription vertex_attributes[] = {
		{
			.binding  = 0,
			.location = 0,
			.format   = VK_FORMAT_R32G32B32_SFLOAT,
			.offset   = offsetof(struct SphereVertex, pos),
		},
		{
			.binding  = 1,
			.location = 1,
			.format   = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset   = offsetof(struct SphereInstance, pos),
		},
		{
			.binding  = 1,
			.location = 2,
			.format   = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset   = offsetof(struct SphereInstance, color),
		}
	};


	/* Pipeline */

	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_SPHERE);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_SPHERE);

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
		.cullMode                = VK_CULL_MODE_FRONT_BIT,
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

	VkDescriptorSetLayout layouts[] = {
		this->scene_layout, 
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = LENGTH(layouts),
		.pSetLayouts = layouts,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
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


void sphere_renderer_destroy_callback(SphereRenderer handle, void*p)
{
	sphere_renderer_destroy(&handle);
}

void sphere_renderer_destroy(SphereRenderer *handle)
{
	struct SphereRenderer *restrict this = handle_deref(&alloc, *handle);

	vkDestroyPipeline (vk.dev, this->pipeline, NULL);
	vkDestroyPipelineLayout(vk.dev, this->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, this->scene_layout, NULL);


	for (size_t i = 0; i < VK_FRAMES; i++) {
		vmaUnmapMemory(vk.vma, this->frame[i].scene_alloc);
		vmaDestroyBuffer(vk.vma, 
			this->frame[i].scene_buffer, 
			this->frame[i].scene_alloc
		);
	}

	vmaDestroyBuffer(vk.vma, 
		this->vertex_buffer, 
		this->vertex_alloc
	);

	for (ufast32_t i = 0; i < SPHERE_LODS; i++) {
		vmaDestroyBuffer(vk.vma, 
			this->lod[i].index_buffer, 
			this->lod[i].index_alloc
		);
	}

	handle_free(&alloc, handle);
}

SphereRenderer sphere_renderer_create(void)
{
	SphereRenderer                  handle = handle_alloc(&alloc);
	struct SphereRenderer *restrict this   = handle_deref(&alloc, handle);


	vk_create(this);


	/* Geometry */

	struct IcosphereMesh *mesh = icosphere_create(0);

	/* lods */
	for (size_t i = 0; i < SPHERE_LODS; i++) {
		vk_upload_buffer(
			&this->lod[i].index_buffer, 
			&this->lod[i].index_alloc, 
			mesh->face, 
			mesh->face_count * sizeof(uint16_t) * 3, 
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		);

		this->lod[i].index_count = mesh->face_count*3;
		
		icosphere_tessellate(mesh);
	}

	/* Geometry */

	VkDeviceSize vksize = mesh->vertex_count * sizeof(struct SphereVertex);
	struct SphereVertex *vertex = calloc(1, vksize);
	for (size_t i = 0; i < mesh->vertex_count; i++) {
		memcpy(&vertex[i].pos, &mesh->vertex[i].pos, 3*sizeof(float));
	}

	vk_upload_buffer(
		&this->vertex_buffer, 
		&this->vertex_alloc, 
		vertex, 
		vksize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
	);

	free(vertex);
	icosphere_destroy(&mesh);


	/* Instance buffer */

	this->sphere_max = 1<<13; // 16384
	this->sphere_count = 0;
	this->sphere = calloc(this->sphere_max, sizeof(*this->sphere));


	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = this->sphere_max * sizeof(struct SphereInstance),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
	};

	for (size_t i = 0; i < VK_FRAMES; i++) {
		struct SphereRendererFrame *frame = this->frame+i;

		VkResult ret = vmaCreateBuffer(vk.vma, 
			&buffer_info, &alloc_info, 
			&frame->instance_buffer, 
			&frame->instance_alloc, 
			NULL
		);
		if(ret != VK_SUCCESS) engine_crash("vmaCreateBuffer failed");

		vmaMapMemory(vk.vma, 
			 this->frame[i].instance_alloc, 
			&this->frame[i].instance_mapping
		);
	}

	/* Cleanup callback */
	event_bind(EVENT_RENDERERS_DESTROY, sphere_renderer_destroy_callback, handle);

	return handle;
}

void sphere_renderer_draw(
	SphereRenderer handle,
	struct Frame *engine_frame)
{
	struct SphereRenderer *restrict this = handle_deref(&alloc, handle);

	uint32_t f          = engine_frame->vk->id;
	VkCommandBuffer cmd = engine_frame->vk->cmd_buf;

	struct SphereRendererFrame *frame = &this->frame[f];

	uint32_t lod = 2;

	/* Scene Uniform */
	static struct SceneUniform scene;
	glm_mat4_copy(engine_frame->camera.projection, scene.proj);
	glm_mat4_copy(engine_frame->camera.view,       scene.view);
	memcpy(this->frame[f].scene_mapping, &scene, sizeof(scene));


	/* Instance buffer */

	size_t bytes = this->sphere_count * sizeof(struct SphereInstance);
	memcpy(
		frame->instance_mapping, 
		this->sphere, 
		bytes
	);
	vmaFlushAllocation(vk.vma, frame->instance_alloc, 0, bytes);


	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

	vkCmdBindVertexBuffers(cmd, 0, 2, 
		(VkBuffer[]){this->vertex_buffer, frame->instance_buffer},
		(VkDeviceSize[]){0, 0}
	);


	vkCmdBindIndexBuffer(cmd, this->lod[lod].index_buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		this->pipeline_layout,
		0, 1,
		&frame->scene_uniform, 0, NULL
	);
	
	vkCmdDrawIndexed(cmd, this->lod[lod].index_count, this->sphere_count, 0, 0, 0);
	this->sphere_count = 0;
}


void sphere_renderer_add(
		SphereRenderer handle,
		float    pos[3],
		float    r,
		uint32_t color)
{
	struct SphereRenderer *restrict this = handle_deref(&alloc, handle);

	if (this->sphere_count >= this->sphere_max) {
		return;
	}

	struct SphereInstance *sphere = &this->sphere[this->sphere_count++];
	
	sphere->color[0] = (color >> 24 & 0xFF) / 255.0;
	sphere->color[1] = (color >> 16 & 0xFF) / 255.0;
	sphere->color[2] = (color >> 8  & 0xFF) / 255.0;
	sphere->color[3] = (color >> 0  & 0xFF) / 255.0;

	sphere->radius = r;
	memcpy(sphere->pos, pos, sizeof(float)*3);
}


