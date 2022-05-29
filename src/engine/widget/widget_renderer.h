#pragma once 

#include "common.h"
#include "gfx/gfx_types.h"
#include "text/text_common.h"

struct WidgetUniform {
	mat4 ortho;
};

struct WidgetVertex {
	float    pos  [2];
	uint8_t  color[4];
	uint16_t depth;
};
typedef Handle WidgetRenderer;
struct WidgetRenderer {

	uint32_t quad_count;
	uint32_t quad_max;

	struct WidgetVertex *vertex_buffer;

	TextEngine                     engine;

	VkPipelineLayout               pipeline_layout;
	VkPipeline                     pipeline;

	VkBuffer                       index_buffer;
	VmaAllocation                  index_alloc;

	struct WidgetFrameData {
		VkBuffer         vertex_buffer;
		VmaAllocation    vertex_alloc;
		void            *vertex_mapping;

		uint32_t         index_count;
	} frame[VK_FRAMES];

};

WidgetRenderer widget_renderer_create (void);
void           widget_renderer_destroy(WidgetRenderer*);
void           widget_renderer_clear  (WidgetRenderer);

void  widget_renderer_quad(WidgetRenderer, 
	int32_t, int32_t, int32_t, int32_t, 
	uint16_t depth,
	uint32_t color);

void           widget_renderer_draw   (WidgetRenderer, struct Frame*);

