#include "term.h"
#include "event/event.h"
#include "log/log.h"
#include "input.h"
#include "engine/text/text.h"

#include "engine/widget/widget_renderer.h"

static TextBlock    blk[64];
static uint32_t     line = 0;
static TextGeometry geom;
static WidgetRenderer gui;

uint32_t focus;
int32_t  cur_x;
int32_t  cur_y;

struct {
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
	int32_t margin;
	int32_t scroll;
	int32_t scroll_bar;
	int32_t scroll_track;
	int32_t content;
} term = {
	.x = 20,
	.y = 100,
	.w = 500,
	.h = 200,
	.scroll = 0,
	.margin = 5,
	.scroll_track = 500-10
};

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

TextGeometry term_create_gfx(TextRenderer renderer, WidgetRenderer widget)
{
	gui = widget;
	geom = text_geometry_create(renderer, 1024*16, TEXT_GEOMETRY_DYNAMIC);
	return geom;
}

void term_create(TextEngine engine)
{

	for (ufast32_t i = 0; i < LENGTH(blk); i++) {
		blk[i] = text_block_create(engine);
		text_block_set_wrap_indent(blk[i], 30);
	}

	log_info("Terminal listening");
	event_bind(EVENT_LOG, log_callback, 0);
}

void term_input(void)
{
	double _x, _y;
	input_cursor_pos(&_x, &_y);
	int32_t x = _x;
	int32_t y = _y;

	int32_t dx = x-cur_x;
	int32_t dy = y-cur_y;

	term.scroll_track = term.h - term.margin*2;
	term.scroll_bar   = term.scroll_track * (term.h / (float)term.content);

	switch (focus) {
	case 1:
		term.x += dx;
		term.y += dy;
		break;
	case 2:
		term.w += dx;
		term.h += dy;
		break;
	case 3:
		term.scroll -= dy;
		break;
	}
	term.w = MAX(10, term.w);
	term.h = MAX(10, term.h);
	term.scroll_bar = MIN(term.scroll_bar, term.scroll_track);


	term.scroll = MIN(term.scroll_track-term.scroll_bar, term.scroll);
	term.scroll = MAX(0,      term.scroll);

	cur_x = x;
	cur_y = y;
}

void scroll_box(int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
	uint32_t scroll_w = 10;

	*x = term.x+term.w - term.margin - scroll_w; 
	*y = term.y+term.h - term.margin - term.scroll - term.scroll_bar;
	*w = scroll_w;
	*h = term.scroll_bar;
}

bool scroll_in(int32_t cx, int32_t cy)
{
	int32_t x, y, w, h;
	scroll_box(&x, &y, &w, &h);

	return (
	    cx >= x
	 && cx <  x+w
	 && cy >= y
	 && cy <  y+h
	);
}

void term_update(struct Frame *frame)
{

	if (focus) term_input();

	text_geometry_clear(geom);

	ufast32_t height = 0;
	for (ufast32_t i = 0; i < LENGTH(blk); i++) {
		text_block_align(blk[i], TEXT_ALIGN_LEFT, term.w - term.margin*4);
		height += MAX(text_block_get_lines(blk[i]), 1) 
			* text_block_get_line_height(blk[i]);
	}

	term.content = height + term.margin*2;


	int32_t x, y, w, h;
	scroll_box(&x, &y, &w, &h);

	widget_renderer_quad(gui,
		x,y,w,h,
		511,
		0xAAAAAAFF
	);

	widget_renderer_quad(gui,
		term.x, term.y, term.w, term.h,
		512,
		0x050505CC
	);

	widget_renderer_quad(gui,
		term.x-1, term.y-1, term.w+2, term.h+2,
		513,
		0x888888FF
	);


	text_geometry_set_scissor(geom, term.x, term.y, term.w, term.h);

	int32_t scroll_offset = term.content*(term.scroll / (float)term.scroll_track);

	text_geometry_set_cursor(geom, term.x + term.margin, 
		(term.y+term.h) - height - term.margin*2 + scroll_offset
	);  

	for (ufast32_t i = 0; i < LENGTH(blk); i++){
		text_geometry_push(geom, blk[(i+line)%LENGTH(blk)] );
		text_geometry_newline(geom);
	}
}


bool term_mouse(int action)
{
	if (action == GLFW_RELEASE) {
		if (focus) {
			focus = false;
			return true;
		}
		return false;
	}

	double dx, dy;
	input_cursor_pos(&dx, &dy);
	int32_t x = dx;
	int32_t y = dy;
	cur_x = x;
	cur_y = y;


	bool in = (x >= term.x
	 && x <  term.x+term.w
	 && y >= term.y
	 && y <  term.y+term.h
	);

	if(!in) {
		focus = 0;
		return false;
	}

	if (scroll_in(x, y)) {
		focus = 3;
		return true;
	}

	if (x > term.x+term.w-20
	 && y > term.y+term.h-20)
		focus = 2;
	 else 
		focus = 1;

	 return true;
}

