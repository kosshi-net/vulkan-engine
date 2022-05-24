#include "engine/engine.h"
#include "engine/win/win.h"
#include "engine/log/log.h"
#include "engine/gfx/gfx.h"
#include "engine/gfx/camera.h"
#include "teapot/teapot.h"
#include "engine/gfx/text/text.h"
#include "engine/gfx/text/vk_text.h"
#include "engine/ppm.h"

#include "input.h"
#include "freecam.h"
#include "term.h"

int main(int argc, char**argv)
{
	log_info("Hello Vulkan!");
	
	engine_init();
	input_init();
	term_create();

	struct {
		struct TextContext *ctx;
		uint32_t            gfx;
		struct TextBlock   *hello;
		struct TextBlock   *stats;
		struct TextBlock   *cam;
		struct TextBlock   *tea;
		struct TextBlock   *ctl_head;
		struct TextBlock   *ctl_right;
		struct TextBlock   *ctl_left;
		int                 ctl_width;
	} txt = {
		.ctl_width = 150
	};

	txt.ctx   = txtctx_create(FONT_SANS_16);
	txt.hello = txtblk_create(txt.ctx, "Hello Vulkan!\n");
	txt.stats = txtblk_create(txt.ctx, NULL);
	txt.cam   = txtblk_create(txt.ctx, NULL);
	txt.tea   = txtblk_create(txt.ctx, NULL);

	struct TextStyle s;
	txtblk_add_text(txt.tea, "Teapot ",       textstyle_set(&s, 0xFFFFFFFF, 0));
	txtblk_add_text(txt.tea, "ティーポット ", textstyle_set(&s, 0x88FFFFFF, 0));
	txtblk_add_text(txt.tea, "чайник ",       textstyle_set(&s, 0xFF88FFFF, 0));
	txtblk_add_text(txt.tea, "ابريق الشاي ",  textstyle_set(&s, 0xFFFF88FF, 0));
	txtblk_add_text(txt.tea, "茶壶 ",         textstyle_set(&s, 0x8888FFFF, 0));
	txtblk_add_text(txt.tea, "קוּמקוּם  ",      textstyle_set(&s, 0xFF8888FF, 0));
	txtblk_add_text(txt.tea, "τσαγιέρα ",     textstyle_set(&s, 0xFF8888FF, 0));
	txtblk_add_text(txt.tea, "\n\n", NULL);

	txtblk_add_text(txt.tea, 
		"Italic text. ティーポット <- if no italic font present, fall back to normal \n", 
		textstyle_set(&s, 0xFFFFFFFF, FONT_STYLE_ITALIC )
	);
	txtblk_add_text(txt.tea, 
		"This is green bold text. ", 
		textstyle_set(&s, 0x44FF44FF, FONT_STYLE_BOLD)
	);
	txtblk_add_text(txt.tea, 
		"This is both italic and bold.\n", 
		textstyle_set(&s, 0xFF4444FF, FONT_STYLE_BOLD | FONT_STYLE_ITALIC)
	);
	
	txtctx_add(txt.hello);

	txt.ctl_head = txtblk_create(txt.ctx, 
		"Controls"
	);
	txt.ctl_left = txtblk_create(txt.ctx, 
		"\n"
		"Exit\n"
		"Lock cursor\n"
		"Unlock cursor\n"
		"Move\n"
		"Up\n"
		"Down\n"
		"Zoom\n"
		"Crash\n"
	);
	txt.ctl_right = txtblk_create(txt.ctx, 
		"\n"
		"Q\n"
		"MB1\n"
		"ESC\n"
		"WASD\n"
		"Space\n"
		"RShift\n"
		"ZX\n"
		"Delete\n"
	);

	txtblk_align(txt.ctl_head,  TEXT_ALIGN_CENTER, txt.ctl_width);
	txtblk_align(txt.ctl_right, TEXT_ALIGN_RIGHT,  txt.ctl_width);
	txtblk_align(txt.ctl_left,  TEXT_ALIGN_LEFT,   txt.ctl_width);

	txt.gfx = gfx_text_renderer_create(txt.ctx, 512);

	uint32_t teagfx= gfx_teapot_renderer_create();

	uint32_t frames = 0;
	uint32_t fps_max = 300;
	uint32_t fps = 0;
	double   last_title = 0.0;
	double   last = 0;
	double   sleep = 0;

	static struct Freecam freecam = {
		.fov = 90.0,
	};

#ifndef NDEBUG
	gfx_util_write_ppm(txt.ctx->atlas.w, txt.ctx->atlas.h, txt.ctx->atlas.bitmap, "atlas.ppm");
#endif

	log_warn("Test warning!");
	log_error("Test error!");

	while (!win_should_close()) {
		
		struct Frame *frame = frame_begin();
		
		double now   = glfwGetTime();
		double delta = now - last; 
		last = now;

		if (last_title + 1.0 < now) {
			fps = frames;
			frames = 0;
			last_title += 1.0;

		}

		txtctx_clear(txt.ctx);
		txtctx_set_root(txt.ctx, 10, 10);
		char print[1024];
		snprintf(print, sizeof(print), 
			"FPS: %i/%i, sleeped %.2fms, busy %.2fms\n",
			fps, fps_max, sleep*1000.0, (delta-sleep)*1000.0
		);
		txtblk_edit(txt.stats, print);
		snprintf(print, sizeof(print), 
			"[%.2f, %.2f, %.2f], [%.1f, %.1f], [%.1f]\n",
			freecam.pos[0], freecam.pos[1], freecam.pos[2], 
			freecam.yaw, freecam.pitch,
			freecam.fov
		);
		txtblk_edit(txt.cam, print);

		txtctx_add(txt.hello);
		txtctx_add(txt.stats);
		txtctx_add(txt.tea);
		txtctx_add(txt.cam);

		txtctx_set_root(txt.ctx, frame->width-txt.ctl_width, 0);
		txtctx_add(txt.ctl_head);
		txtctx_set_root(txt.ctx, frame->width-txt.ctl_width, 0);
		txtctx_add(txt.ctl_right);
		txtctx_set_root(txt.ctx, frame->width-txt.ctl_width, 0);
		txtctx_add(txt.ctl_left);

		/* Camera stuff */
		freecam_update(&freecam, delta);
		freecam_to_camera(&freecam, &frame->camera);
		glm_perspective(
			glm_rad(freecam.fov),
			frame->width / (float) frame->height,
			0.1f, 1000.0f,
			frame->camera.projection
		);
		frame->camera.projection[1][1] *= -1; 

		gfx_teapot_draw(frame);
		gfx_text_draw(frame, txt.gfx);
		term_update(frame);

		frame_end(frame);
		engine_tick();

		sleep = (1.0f/(float)fps_max) - (glfwGetTime()-now);
		if(sleep > 0.0) {
			usleep(980*1000*sleep);
		}
		frames++;
	}

	engine_wait_idle();
	term_destroy();
	gfx_teapot_renderer_destroy(teagfx);
	gfx_text_renderer_destroy(txt.gfx);

	engine_destroy();

	return 0;
}
