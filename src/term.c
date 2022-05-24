#include "term.h"
#include "event/event.h"
#include "log/log.h"
#include "gfx/text/text.h"
#include "gfx/text/vk_text.h"

static struct TextContext *txtctx;
static struct TextBlock   *txtblk[24];
static uint32_t            line = 0;
static TextRenderer        gfx;

struct TextStyle styles[] = {
	[LOG_DEBUG] = {.color = {0xAA, 0xAA, 0xFF, 0xFF}},
	[LOG_INFO]  = {.color = {0xFF, 0xFF, 0xFF, 0xFF}},
	[LOG_WARN]  = {.color = {0xFF, 0xFF, 0x00, 0xFF}},
	[LOG_ERROR] = {.color = {0xFF, 0x00, 0x00, 0xFF}},
};

void log_callback(Handle handle, void*arg) 
{
	struct LogEvent *log = arg;
	txtblk_set_text(txtblk[line], log->message, &styles[log->level]);
	txtblk_align(txtblk[line], TEXT_ALIGN_LEFT, 800);
	line++;
	line %= LENGTH(txtblk);
}

void term_create(void)
{
	txtctx = txtctx_create(FONT_MONO_16);
	txtctx->mute_logging = true; /* Mute by default, creates loops otherwise */
	gfx    = gfx_text_renderer_create(txtctx, 1024*16);

	for (ufast32_t i = 0; i < LENGTH(txtblk); i++)
		txtblk[i] = txtblk_create(txtctx, NULL);

	log_info("Terminal created");
	event_bind(EVENT_LOG, log_callback, 0);
}

void term_update(struct Frame *frame)
{
	txtctx_clear(txtctx);

	ufast32_t lines = 0;
	for (ufast32_t i = 0; i < LENGTH(txtblk); i++) {
		lines += txtblk[i]->lines;
	}

	txtctx_set_scissor(txtctx, 0, 0, frame->width, frame->height);
	txtctx_set_root(txtctx, 0, frame->height - lines*txtctx->font_size-4); 

	for (ufast32_t i = 0; i < LENGTH(txtblk); i++){
		txtctx_add( txtblk[(i+line)%LENGTH(txtblk)] );
		txtctx_newline(txtctx);
	}

	gfx_text_draw(frame, gfx);
}

void term_destroy(void)
{
	gfx_text_renderer_destroy(gfx);
}

