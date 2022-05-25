#pragma once

#include "common.h"
#include "gfx/gfx_types.h"
#include "text/text_common.h"

#define TEXT_VERTICES_PER_GLYPH 4
#define TEXT_INDICES_PER_GLYPH 6

struct TextVertex {
	float   pos  [2];
	float   uv   [2];
	uint8_t color[4];
};

struct TextUBO {
	mat4 ortho;
};

enum TextGeometryType {
	TEXT_GEOMETRY_STATIC,
	TEXT_GEOMETRY_DYNAMIC,
};

struct TextGeometry {
	bool valid;

	enum TextGeometryType type;
	uint32_t              frame_count;
	struct TextFrameData {
		VkBuffer         uniform_buffer;
		VmaAllocation    uniform_alloc;
		VkDescriptorSet  descriptor_set;

		VkBuffer         vertex_buffer;
		VmaAllocation    vertex_alloc;
		void            *vertex_mapping;

		uint32_t         index_count;
	} frame[VK_FRAMES];


	int32_t root_x;
	int32_t root_y;

	int32_t cursor_x;
	int32_t cursor_y;

	int32_t line_height;

	bool scissor_enable;

	struct {
		int32_t x; 
		int32_t y;
		int32_t w;
		int32_t h;
	} scissor;

	Array(struct TextVertex) vertex_buffer;
	uint32_t glyph_count;
	uint32_t glyph_max;
};

TextGeometry text_geometry_create(TextRenderer, size_t, enum TextGeometryType);
TextGeometry text_geometry_destroy(TextGeometry);

struct TextGeometry *text_geometry_get_struct(TextEngine handle);

void text_geometry_push(TextGeometry, TextBlock);
void text_geometry_clear(TextGeometry);
void text_geometry_newline(TextGeometry);
void text_geometry_set_cursor(TextGeometry, int32_t, int32_t);
void text_geometry_set_scissor(TextGeometry,
	int32_t x, int32_t y, int32_t w, int32_t h);

/* Automatically called for TEXT_GEOMETRY_DYNAMIC by text_renderer_draw */
void text_geometry_upload(TextGeometry, struct Frame *);
