#include "engine/engine.h"
#include "engine/win/win.h"
#include "engine/log/log.h"
#include "engine/gfx/gfx.h"
#include "engine/gfx/camera.h"
#include "teapot/teapot.h"
#include "engine/text/text.h"
#include "engine/ppm.h"

#include "input.h"
#include "freecam.h"
#include "term.h"

int main(int argc, char**argv)
{
	log_info("Hello Vulkan!");
	

	struct {
		TextEngine   engine;
		TextBlock    block;
		TextBlock    perf;

		TextRenderer gfx;
		TextGeometry geom_static;
		TextGeometry geom;

		TextBlock    ctl_h;
		TextBlock    ctl_r;
		TextBlock    ctl_l;
		uint32_t     ctl_w;
	} txt = {
		.ctl_w = 200
	};
	
	txt.engine = text_engine_create();
	term_create(txt.engine);

	engine_init();
	input_init();

	txt.block  = text_block_create(txt.engine);
	txt.perf   = text_block_create(txt.engine);
	txt.gfx = text_renderer_create(txt.engine);
	txt.geom = text_geometry_create(txt.gfx, 1024, TEXT_GEOMETRY_DYNAMIC);
	txt.geom_static = text_geometry_create(txt.gfx, 1024, TEXT_GEOMETRY_STATIC);

	term_create_gfx(txt.gfx);

	struct TextStyle s;
	text_block_add(txt.block, "Hello Vulkan!\n", NULL);
	text_block_add(txt.block, "Teapot ",       textstyle_set(&s, 0xFFFFFFFF, 0));
	text_block_add(txt.block, "ティーポット ", textstyle_set(&s, 0x44FFFFFF, 0));
	text_block_add(txt.block, "чайник ",       textstyle_set(&s, 0xFF44FFFF, 0));
	text_block_add(txt.block, "ابريق الشاي ",  textstyle_set(&s, 0xFFFF44FF, 0));
	text_block_add(txt.block, "茶壶 ",         textstyle_set(&s, 0xFF4444FF, 0));
	text_block_add(txt.block, "קוּמקוּם  ",      textstyle_set(&s, 0x4444FFFF, 0));
	text_block_add(txt.block, "τσαγιέρα ",     textstyle_set(&s, 0x44FF44FF, 0));
	text_block_add(txt.block, "\n\n", NULL);

	textstyle_set(&s, 0xFF4444FF, FONT_STYLE_ITALIC);
	text_block_add(txt.block, "Italic ", &s);

	textstyle_set(&s, 0x44FF44FF, FONT_STYLE_BOLD);
	text_block_add(txt.block, "Bold ", &s); 

	textstyle_set(&s, 0x4444FFFF, FONT_STYLE_ITALIC|FONT_STYLE_BOLD);
	text_block_add(txt.block, "BoldItalic", &s);

	text_geometry_set_cursor(txt.geom_static, 32, 64);
	text_geometry_push(txt.geom_static, txt.block);
	text_geometry_upload(txt.geom_static, NULL);


	txt.ctl_h = text_block_create(txt.engine);
	txt.ctl_r = text_block_create(txt.engine);
	txt.ctl_l = text_block_create(txt.engine);

	textstyle_set(&s, 0xFFFFFFFF, FONT_STYLE_ITALIC);
	text_block_set(txt.ctl_h, "Controls", &s);
	text_block_set(txt.ctl_l,
		"\n"
		"Exit\n"
		"Lock cursor\n"
		"Unlock cursor\n"
		"Move\n"
		"Up\n"
		"Down\n"
		"Zoom\n"
		"Crash\n",
		NULL
	);
	text_block_set(txt.ctl_r,
		"\n"
		"Q\n"
		"MB1\n"
		"Escape\n"
		"WASD\n"
		"Space\n"
		"RShift\n"
		"ZX\n"
		"Delete\n",
		NULL
	);
	text_block_align(txt.ctl_h, TEXT_ALIGN_CENTER, txt.ctl_w);
	text_block_align(txt.ctl_l, TEXT_ALIGN_LEFT,   txt.ctl_w);
	text_block_align(txt.ctl_r, TEXT_ALIGN_RIGHT,  txt.ctl_w);

	uint32_t teagfx = gfx_teapot_renderer_create();

	uint32_t frames = 0;
	uint32_t fps_max = 300;
	uint32_t fps = 0;
	double   last_title = 0.0;
	double   last = 0;
	double   sleep = 0;

	static struct Freecam freecam = {
		.fov = 90.0,
	};

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

		/* Text stuff */
		char print[1024];
		snprintf(print, sizeof(print), 
			"FPS: %i/%i, sleeped %.2fms, busy %.2fms\n"
			"[%.2f, %.2f, %.2f], [%.1f, %.1f], [%.1f]\n",
			fps, fps_max, sleep*1000.0, (delta-sleep)*1000.0,
			freecam.pos[0], freecam.pos[1], freecam.pos[2], 
			freecam.yaw, freecam.pitch,
			freecam.fov
		);
		text_block_set(txt.perf, print, textstyle_set(&s, 0xFFFFFFFF, FONT_STYLE_MONOSPACE));
		text_geometry_clear(txt.geom);
		text_geometry_push(txt.geom, txt.perf);

		text_geometry_set_cursor(txt.geom, frame->width-txt.ctl_w, 0);
		text_geometry_push(txt.geom, txt.ctl_h);
		text_geometry_set_cursor(txt.geom, frame->width-txt.ctl_w, 0);
		text_geometry_push(txt.geom, txt.ctl_r);
		text_geometry_set_cursor(txt.geom, frame->width-txt.ctl_w, 0);
		text_geometry_push(txt.geom, txt.ctl_l);

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

		/* Render stuff */
		gfx_teapot_draw(frame);
		term_update(frame);
		text_renderer_draw(txt.gfx, txt.geom, frame);
		text_renderer_draw(txt.gfx, txt.geom_static, frame);

		/* End stuff */
		frame_end(frame);
		engine_tick();

		sleep = (1.0f/(float)fps_max) - (glfwGetTime()-now);
		if(sleep > 0.0) {
			usleep(980*1000*sleep);
		}
		frames++;
	}
	text_engine_export_atlas(txt.engine);

	engine_wait_idle();
	gfx_teapot_renderer_destroy(teagfx);

	engine_destroy();

	return 0;
}
