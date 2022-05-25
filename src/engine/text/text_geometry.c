#include "text/text_geometry.h"
#include "text/text_renderer.h"
#include "text/text_block.h"
#include "text/text_engine.h"
#include "common.h"
#include "event/event.h"
#include "handle/handle.h"

#include "gfx/gfx.h"
#include "gfx/vk_util.h"

extern struct VkEngine vk;


static struct HandleAllocator alloc = HANDLE_ALLOCATOR(struct TextGeometry, 8);

struct TextGeometry *text_geometry_get_struct(TextGeometry handle)
{
	return handle_dereference(&alloc, handle);
}

void text_geometry_destroy_callback(TextGeometry handle, void*arg)
{
	text_geometry_destroy(handle);
}

TextGeometry text_geometry_destroy(TextGeometry handle)
{
	struct TextGeometry *restrict this = text_geometry_get_struct(handle);
	
	for (uint32_t i = 0; i < this->frame_count; i++) {
		vmaUnmapMemory(vk.vma, this->frame[i].vertex_alloc);
		vmaDestroyBuffer(vk.vma, 
			this->frame[i].vertex_buffer, 
			this->frame[i].vertex_alloc
		);

		vmaDestroyBuffer(vk.vma, 
			this->frame[i].uniform_buffer, this->frame[i].uniform_alloc
		);
	}

	this->valid = false;
	return 0;
}

TextGeometry text_geometry_create(
	TextRenderer          gfx_handle, 
	size_t                max_glyphs,
	enum TextGeometryType type)
{
	TextGeometry                  handle = handle_allocate(&alloc);
	struct TextGeometry *restrict this   = text_geometry_get_struct(handle);
	struct TextRenderer *restrict gfx    = text_renderer_get_struct(gfx_handle);

	this->line_height = 0;
	this->glyph_max   = max_glyphs;
	this->type        = type;
	this->frame_count = type == TEXT_GEOMETRY_STATIC ? 1 : VK_FRAMES;

	array_create(this->vertex_buffer);

	/*****************
	 * Create frames *
	 *****************/

	VkResult ret; 

	size_t max_vertices = this->glyph_max * 4;
	size_t size = max_vertices * sizeof(float) * 6;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
	};
	for (ufast32_t i = 0; i < this->frame_count; i++) {
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
			.pSetLayouts        = &gfx->descriptor_layout
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
			.imageView   = gfx->texture_view,
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

	event_bind(EVENT_RENDERERS_DESTROY, text_geometry_destroy_callback, handle);

	return handle;
}

void text_geometry_newline(TextGeometry handle)
{
	struct TextGeometry *restrict this   = text_geometry_get_struct(handle);
	this->cursor_x = 0;
	this->cursor_y += this->line_height;
}

void text_geometry_set_cursor(TextGeometry handle, int32_t x, int32_t y)
{
	struct TextGeometry *restrict this   = text_geometry_get_struct(handle);
	this->cursor_x = 0;
	this->cursor_y = 0;
	this->root_x = x;
	this->root_y = y;
}


void text_geometry_clear(TextGeometry handle)
{
	struct TextGeometry *restrict this   = text_geometry_get_struct(handle);
	array_clear( this->vertex_buffer );
	this->cursor_x = 0;
	this->cursor_y = 0;
	this->root_x = 0;
	this->root_y = 0;
	this->glyph_count = 0;
}


void text_geometry_set_scissor(
	TextGeometry handle,
	int32_t x, int32_t y, int32_t w, int32_t h)
{
	struct TextGeometry *restrict this   = text_geometry_get_struct(handle);
	this->scissor.x = x;
	this->scissor.y = y;
	this->scissor.w = w;
	this->scissor.h = h;
	this->scissor_enable = true;
}

void text_geometry_push (TextGeometry handle, TextBlock block_handle)
{
	struct TextGeometry *restrict this   = text_geometry_get_struct(handle);
	struct TextBlock    *restrict block  = text_block_get_struct(block_handle);
	struct TextEngine   *restrict engine = text_engine_get_struct(block->engine);

	this->line_height = block->line_height;

	static const int32_t quad_vertex[4][3] = {
		{0, 0, 0}, 
		{0, 1, 0}, 
		{1, 0, 0}, 
		{1, 1, 0}, 
	};

	struct TextVertex *vert = array_reserve(this->vertex_buffer, 4);

	for (ufast32_t g = 0; g < array_length(block->quads); g++)
	{
		struct GlyphQuad *restrict quad = &block->quads[g];
		int32_t x = quad->offset_x + this->root_x + this->cursor_x;
		int32_t y = quad->offset_y + this->root_y + this->cursor_y 
			+ this->line_height;

		this->cursor_x += quad->advance_x;
		this->cursor_y += quad->advance_y;

		if (quad->newline) {
			this->cursor_x = 0;
			this->cursor_y += this->line_height;
		}

		if (quad->space) 
			continue;

		int32_t w = quad->slot->w;
		int32_t h = quad->slot->h;

		uint32_t out_of_bounds =
			(x   <  this->scissor.x) +
			(y   <  this->scissor.y) +
			(x+w >= this->scissor.w) +
			(y+h >= this->scissor.h);

		if (this->scissor_enable && out_of_bounds == 4) continue;

		float uv_x = quad->slot->atlas_x / (float)engine->atlas.w;
		float uv_y = quad->slot->atlas_y / (float)engine->atlas.h;
		float uv_w = w / (float)engine->atlas.w;
		float uv_h = h / (float)engine->atlas.h;
		
		for (int i = 0; i < 4; i++) {
			vert[i].pos[0] = (x + w * quad_vertex[i][0]);
			vert[i].pos[1] = (y + h * quad_vertex[i][1]);

			vert[i].uv[0] = (uv_x + uv_w * (float)quad_vertex[i][0]);
			vert[i].uv[1] = (uv_y + uv_h * (float)quad_vertex[i][1]);

			memcpy(vert[i].color, quad->color, sizeof(vert[i].color));
		}

		this->glyph_count++;
		vert = array_reserve(this->vertex_buffer, 4);
	}
	array_pop(this->vertex_buffer, 4);
}

void text_geometry_upload(
	TextGeometry handle, 
	struct Frame *restrict frame) 
{
	struct TextGeometry *restrict this = text_geometry_get_struct(handle);
	
	uint32_t f = (this->type == TEXT_GEOMETRY_STATIC) ? 0 : frame->vk->id;

	size_t glyphs = this->glyph_count;

	if (glyphs > this->glyph_max) {
		log_error("Max glyphs exceeded (%li > %li)", 
			glyphs, this->glyph_max
		);
		glyphs = this->glyph_max;
	}

	size_t bytes = glyphs * TEXT_VERTICES_PER_GLYPH * sizeof(struct TextVertex);
	memcpy(
		this->frame[f].vertex_mapping, 
		this->vertex_buffer, 
		bytes
	);

	vmaFlushAllocation(vk.vma, this->frame[f].vertex_alloc, 0, bytes);
}

uint16_t *txt_create_shared_index_buffer(size_t max_glyphs, size_t *size)
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

