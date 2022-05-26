#include "term.h"
#include "event/event.h"
#include "log/log.h"
#include "engine/text/text.h"

static TextBlock    blk[24];
static uint32_t     line = 0;
static TextGeometry geom;
static TextRenderer gfx;

struct TextStyle styles[] = {
	[LOG_DEBUG] = {.color = {0xAA, 0xAA, 0xFF, 0xFF}, .style = FONT_STYLE_MONOSPACE},
	[LOG_INFO]  = {.color = {0xFF, 0xFF, 0xFF, 0xFF}, .style = FONT_STYLE_MONOSPACE},
	[LOG_WARN]  = {.color = {0xFF, 0xFF, 0x00, 0xFF}, .style = FONT_STYLE_MONOSPACE},
	[LOG_ERROR] = {.color = {0xFF, 0x00, 0x00, 0xFF}, .style = FONT_STYLE_MONOSPACE},
};

void log_callback(Handle handle, void*arg) 
{
	struct LogEvent *log = arg;
	text_block_set  (blk[line], log->message, &styles[log->level]);
	text_block_align(blk[line], TEXT_ALIGN_LEFT, 800);
	line++;
	line %= LENGTH(blk);
}

void term_create_gfx(TextRenderer renderer)
{
	gfx  = renderer;
	geom = text_geometry_create(gfx, 1024*16, TEXT_GEOMETRY_DYNAMIC);
}

void term_create(TextEngine engine)
{

	for (ufast32_t i = 0; i < LENGTH(blk); i++) {
		blk[i] = text_block_create(engine);
		text_block_set_wrap_indent(blk[i], 20);
	}

	log_info("Terminal listening");
	event_bind(EVENT_LOG, log_callback, 0);
}

void term_update(struct Frame *frame)
{
	text_geometry_clear(geom);

	ufast32_t height = 0;
	for (ufast32_t i = 0; i < LENGTH(blk); i++) {
		height += MAX(text_block_get_lines(blk[i]), 1) 
			* text_block_get_line_height(blk[i]);
	}

	text_geometry_set_scissor(geom, 0, 0, frame->width, frame->height);
	text_geometry_set_cursor(geom, 0, frame->height - height-4);  

	for (ufast32_t i = 0; i < LENGTH(blk); i++){
		text_geometry_push(geom, blk[(i+line)%LENGTH(blk)] );
		text_geometry_newline(geom);
	}

	text_renderer_draw(gfx, geom, frame);
}

