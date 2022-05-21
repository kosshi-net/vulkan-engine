#include "gfx/gfx.h"
#include "gfx/vk_util.h"
#include "gfx/teapot/teapot.h"
#include "gfx/camera.h"
#include "common.h"
#include "log/log.h"
#include "array.h"
#include "event/event.h"
#include "res.h"

#include <fast_obj.h>
#include <stb_image.h>

/**************
 * TEMP UTILS *
 **************/

float urandf(){
	return (rand()/(float)RAND_MAX);
}

float randf(){
	return (rand()/(float)RAND_MAX)*2.0-1.0;
}



static struct TeapotRenderer *this;
extern struct VkEngine vk;

/**********
 * SCENE *
 **********/

static struct SceneUBO scene_ubo;

struct TestScene {
	int object_num;
	float *seeds;
};

static struct TestScene scene = {
	.object_num = 9,
};

/**********
 * TEAPOT *
 **********/

Array(float)    pot_vertex;
Array(uint16_t) pot_index;

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

void init_scene(){
	int val_num = scene.object_num * 9;
	scene.seeds = malloc( sizeof(float) * val_num );
	for(int i = 0; i < val_num; i++){
		scene.seeds[i] = randf();
	}
}

void vk_load_teapot(void){

	fastObjMesh *mesh = fast_obj_read("teapot.obj");

	log_info("Loaded utah teapot");
	log_info("Groups:    %i", mesh->group_count);
	log_info("Objects:   %i", mesh->object_count);
	log_info("Materials: %i", mesh->material_count);
	log_info("Faces      %i", mesh->face_count);
	log_info("Positions  %i", mesh->position_count);
	log_info("Indices    %i", mesh->index_count);
	log_info("Normals    %i", mesh->normal_count);
	log_info("UVs        %i", mesh->texcoord_count);

	int idx = 0;
	array_create(pot_vertex);
	array_create(pot_index);

	for ( int i = 0; i < mesh->group_count; i++ ){
		log_info("Group %i) %s", i, mesh->groups[i].name);

		fastObjGroup *group = &mesh->groups[i];
		int gidx = 0;

		for (int fi = 0; fi < group->face_count; fi++) {
			
			uint32_t face_verts = mesh->face_vertices[ group->face_offset + fi ];

			switch (face_verts) {
				case 3:
					array_push(pot_index, idx+0 );
					array_push(pot_index, idx+1 );
					array_push(pot_index, idx+2 );
					break;
				case 4:
					array_push(pot_index,  idx+0 );
					array_push(pot_index,  idx+1 );
					array_push(pot_index,  idx+2 );

					array_push(pot_index, idx+2);
					array_push(pot_index, idx+3);
					array_push(pot_index, idx+0);
					break;
				default:
					log_error("UNSUPPORTED POLYGON %i\n", face_verts);
					break;
			}

			
			for( int ii = 0; ii < face_verts; ii++ ){
				fastObjIndex mi = mesh->indices[group->index_offset + gidx];
					
				array_push(pot_vertex, mesh->positions[3 * mi.p + 0]);
				array_push(pot_vertex, mesh->positions[3 * mi.p + 1]);
				array_push(pot_vertex, mesh->positions[3 * mi.p + 2]);

				array_push(pot_vertex, mesh->texcoords[2 * mi.t + 0]);
				array_push(pot_vertex, -mesh->texcoords[2 * mi.t + 1]);

				array_push(pot_vertex, 1.0);

				array_push(pot_vertex, mesh->normals[3 * mi.n + 0]);
				array_push(pot_vertex, mesh->normals[3 * mi.n + 1]);
				array_push(pot_vertex, mesh->normals[3 * mi.n + 2]);

				gidx++;
				idx++;
			}
		}
	}

	log_info("Pot floats: %li", array_length(pot_vertex) );
	log_info("Pot indices: %i", idx);

	fast_obj_destroy(mesh);
}

/************
 * PIPELINE *
 ************/

void vk_create_pipeline()
{
	VkShaderModule vert_shader = vk_create_shader_module(RES_SHADER_VERT_TEST);
	VkShaderModule frag_shader = vk_create_shader_module(RES_SHADER_FRAG_TEST);

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

	VkDescriptorSetLayout layouts[] = { 
		this->scene_layout,
		this->object_layout,
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

	if(ret != VK_SUCCESS) engine_crash("vkCreatePipelineLayout failed");

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
		.layout              = this->pipeline_layout,
		.renderPass          = vk.renderpass,
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1,
	};
	
	ret = vkCreateGraphicsPipelines(vk.dev, 
		VK_NULL_HANDLE, 1, &pipeline_info, NULL, &this->pipeline
	);

	if(ret != VK_SUCCESS) engine_crash("vkCreateGraphicsPipelines failed");

    vkDestroyShaderModule(vk.dev, frag_shader, NULL);	
    vkDestroyShaderModule(vk.dev, vert_shader, NULL);	
	return;
}

void vk_create_scene_layout()
{
	VkResult ret;

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

	ret = vkCreateDescriptorSetLayout(vk.dev, 
		&layout_info, NULL, &this->scene_layout
	);

	if (ret != VK_SUCCESS) engine_crash("vkCreateDescriptorSetLayout failed");
}


void vk_create_object_layout()
{
	VkResult ret;

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

	ret = vkCreateDescriptorSetLayout(vk.dev, 
		&layout_info, NULL, &this->object_layout
	);

	if (ret != VK_SUCCESS) engine_crash("vkCreateDescriptorSetLayout");
}

/*************
 * FRAMEDATA *
 *************/

void teapot_frame_create( struct TeapotFrameData *restrict frame )
{
	VkResult ret;

	frame->object_num = 0;

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

	VkDescriptorBufferInfo uniform_buffer_info = {
		.buffer = frame->uniform_buffer,
		.offset = 0,
		.range  = sizeof(struct SceneUBO)
	};
	VkDescriptorImageInfo uniform_image_info = {
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView   = this->texture_view,
		.sampler     = vk.texture_sampler,
	};

	/*
	 * Scene Descriptor
	 */
	
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
			.pBufferInfo        = &uniform_buffer_info,
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
			.pImageInfo         = &uniform_image_info,
			.pTexelBufferView   = NULL,
		},
	};
	vkUpdateDescriptorSets(vk.dev, LENGTH(desc_write), desc_write, 0, NULL);

	/*
	 * Object Descriptor
	 */

	VkDescriptorSetAllocateInfo dyndesc_ainfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = vk.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &this->object_layout
	};
	ret = vkAllocateDescriptorSets(vk.dev, &dyndesc_ainfo, &frame->object_descriptor);
	if(ret != VK_SUCCESS) engine_crash("vkAllocateDescriptorSets failed");

failure:
	return;
}

void teapot_frame_destroy(struct TeapotFrameData *restrict frame)
{
	vmaDestroyBuffer(vk.vma, frame->uniform_buffer, frame->uniform_alloc);

	if(frame->object_num){
		vmaDestroyBuffer(vk.vma, frame->object_buffer, frame->object_alloc);
	}
}

/************
 * UNIFORMS *
 ************/

void vk_update_object_buffer(struct TeapotFrameData *frame)
{
	double now   = glfwGetTime();

	/* Clear previous frame */
	if(frame->object_num){
		vmaDestroyBuffer(vk.vma, frame->object_buffer, frame->object_alloc);
	}
	frame->object_num = 0;
	
	if( scene.object_num == 0 ) return;
	frame->object_num = scene.object_num;

	/* Allocate */
	struct ObjectUBO *object_buffer = malloc( sizeof(struct ObjectUBO) * scene.object_num );

	vk_create_buffer_vma(
		sizeof(struct ObjectUBO) * scene.object_num,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		&frame->object_buffer,
		&frame->object_alloc
	);

	/* Fill */
	for( int i = 0; i < scene.object_num; i++ ){
		mat4 model; 
		glm_mat4_identity(model);

		float *s = scene.seeds+(i*9);

		float t = now*0.3;
		float x = sin(t+i*0.7)*5.0;
		float z = cos(t+i*0.7)*5.0;
		float y = s[8]+1.5;

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
			.dstSet             = frame->object_descriptor,
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

void vk_update_uniform_buffer(struct TeapotFrameData *frame, struct Camera *camera)
{
	glm_mat4_copy(camera->view,       scene_ubo.view);
	glm_mat4_copy(camera->projection, scene_ubo.proj);

	void *data;
	vmaMapMemory(vk.vma, frame->uniform_alloc, &data);
	memcpy(data, &scene_ubo, sizeof(scene_ubo));
	vmaUnmapMemory(vk.vma, frame->uniform_alloc);
}

void gfx_teapot_draw(struct Frame *restrict engine_frame)
{
	struct TeapotFrameData *frame = &this->frame[engine_frame->vk->id];

	vk_update_object_buffer(frame);

	vk_update_uniform_buffer(frame, &engine_frame->camera);

	VkCommandBuffer cmd = engine_frame->vk->cmd_buf;

	vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline );

	vkCmdBindVertexBuffers(cmd, 0, 1, &this->vertex_buffer, (VkDeviceSize[]){0});
	vkCmdBindIndexBuffer(cmd, this->index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
		this->pipeline_layout,
		0, 1,
		&frame->scene_descriptor, 0, NULL
	);
	
	for (int i = 0; i < scene.object_num; i++) {
		uint32_t dynamic_offset = sizeof(struct ObjectUBO)*i;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
			this->pipeline_layout,
			1, 1,
			&frame->object_descriptor, 
			
			1, 
			&dynamic_offset
		);

		vkCmdDrawIndexed(cmd, array_length(pot_index), 1, 0, 0, 0);
	}
}

/***********
 * TEXTURE *
 ***********/

void vk_create_texture(void)
{
	int texw, texh, texch;
	stbi_uc *pixels = stbi_load("texture.png", &texw, &texh, &texch, STBI_rgb_alpha);
	
	if (!pixels) engine_crash("stbi_load failed (missing file?)");

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
		&this->texture_image,
		&this->texture_alloc
	);

	vk_transition_image_layout(this->texture_image, 
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	vk_copy_buffer_to_image(vk.staging_buffer, this->texture_image, texw, texh);

	vk_transition_image_layout(this->texture_image,
		VK_FORMAT_R8G8B8A8_SRGB, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	vmaDestroyBuffer(vk.vma, vk.staging_buffer, vk.staging_alloc);
}


void vk_create_texture_view()
{
	VkImageViewCreateInfo create_info = {
		.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image    = this->texture_image,
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

	VkResult ret = vkCreateImageView(vk.dev, &create_info, NULL, &this->texture_view);

	if (ret != VK_SUCCESS) engine_crash("vkCreateImageView failed");
}

void gfx_teapot_swapchain_deps_create(void*arg)
{
	vk_create_pipeline();
}

void gfx_teapot_swapchain_deps_destroy(void*arg)
{
	vkDestroyPipeline(vk.dev, this->pipeline, NULL);
	vkDestroyPipelineLayout(vk.dev, this->pipeline_layout, NULL);
}


uint32_t gfx_teapot_renderer_create(void)
{
	this = calloc(sizeof(*this), 1);

	vk_load_teapot();
	init_scene();

	vk_create_scene_layout();
	vk_create_object_layout();
	vk_create_texture();
	vk_create_texture_view();

	vk_upload_buffer( 
		&this->vertex_buffer, &this->vertex_alloc,  
		pot_vertex, array_sizeof(pot_vertex),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
	);

	vk_upload_buffer( 
		&this->index_buffer, &this->index_alloc,  
		pot_index, array_sizeof(pot_index),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	gfx_teapot_swapchain_deps_create(NULL);

	for (int i = 0; i < VK_FRAMES; i++) {
		teapot_frame_create(&this->frame[i]);
	}

	event_bind(EVENT_VK_SWAPCHAIN_DESTROY, &gfx_teapot_swapchain_deps_destroy);
	event_bind(EVENT_VK_SWAPCHAIN_CREATE,  &gfx_teapot_swapchain_deps_create);

	return 1;
}

void gfx_teapot_renderer_destroy(uint32_t handle)
{
	event_unbind(EVENT_VK_SWAPCHAIN_DESTROY, &gfx_teapot_swapchain_deps_destroy);
	event_unbind(EVENT_VK_SWAPCHAIN_CREATE,  &gfx_teapot_swapchain_deps_create);

	gfx_teapot_swapchain_deps_destroy(NULL);

	for (int i = 0; i < VK_FRAMES; i++) {
		teapot_frame_destroy(&this->frame[i]);
	}

	vkDestroyImageView(vk.dev, this->texture_view, NULL);
	vmaDestroyImage( vk.vma, this->texture_image, this->texture_alloc);

	vkDestroyDescriptorSetLayout(vk.dev, this->scene_layout, NULL);
	vkDestroyDescriptorSetLayout(vk.dev, this->object_layout, NULL);

	vmaDestroyBuffer(vk.vma, this->vertex_buffer, this->vertex_alloc);

	vmaDestroyBuffer(vk.vma, this->index_buffer, this->index_alloc);

}
