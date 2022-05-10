#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "engine/engine.h"

#include "engine/win/win.h"
#include "engine/gfx/text/text.h"
#include "engine/gfx/text/vk_text.h"

#include "engine/gfx/gfx.h"

static char teapasta[] = "Teapot\nteapot is a vessel used for steeping tea leaves or a herbal mix in boiling or near-boiling water, and for serving the resulting infusion which is called tea. It is one of the core components of teaware. Dry tea is available either in tea bags or as loose tea, in which case a tea infuser or tea strainer may be of some assistance, either to hold the leaves as they steep or to catch the leaves inside the teapot when the tea is poured. Teapots usually have an opening with a lid at their top, where the dry tea and hot water are added, a handle for holding by hand and a spout through which the tea is served. Some teapots have a strainer built-in on the inner edge of the spout. A small air hole in the lid is often created to stop the spout from dripping and splashing when tea is poured. In modern times, a thermally insulating cover called a tea cosy may be used to enhance the steeping process or to prevent the contents of the teapot from cooling too rapidly.\n";

int main(int argc, char**argv)
{
	engine_init();

	struct {
		struct TextContext *ctx;
		struct TextBlock   *hello;
		struct TextBlock   *stats;
		struct TextBlock   *tea;
		struct TextBlock   *al;
		struct TextBlock   *ac;
		struct TextBlock   *ar;
	} txt;

	txt.ctx   = txtctx_create();
	txt.hello = txtblk_create(txt.ctx, "Hello Vulkan!\n");
	txt.stats = txtblk_create(txt.ctx, NULL);
	txt.tea   = txtblk_create(txt.ctx, 
		"Teapot ティーポット чайник ابريق الشاي 茶壶 קוּמקוּם τσαγιέρα\n"
	);


	txt.al = txtblk_create(txt.ctx, NULL);
	txt.ac = txtblk_create(txt.ctx, NULL);
	txt.ar = txtblk_create(txt.ctx, NULL);

	txt.al->max_width = 800;
	txt.ac->max_width = 800;
	txt.ar->max_width = 800;

	txt.al->align = TEXT_ALIGN_LEFT;
	txt.ac->align = TEXT_ALIGN_CENTER;
	txt.ar->align = TEXT_ALIGN_RIGHT;

	txtblk_edit(txt.al, teapasta);
	txtblk_edit(txt.ac, teapasta);
	txtblk_edit(txt.ar, teapasta);


	txtctx_add(txt.hello);
	vk_text_add_ctx(txt.ctx);
	gfx_init(); // TODO !!!!

	uint32_t frames = 0;
	uint32_t fps_max = 300;
	uint32_t fps = 0;
	double   last_title = 0.0;
	double   last = 0;
	double   sleep = 0;
	while (!win_should_close()) {
		
		double now   = glfwGetTime();
		double delta = now - last; 
		last = now;

		if (last_title + 1.0 < now) {
			fps = frames;
			frames = 0;
			last_title += 1.0;
		}

		txtctx_clear(txt.ctx);

		char print[1024];
		snprintf(print, sizeof(print), "FPS: %i/%i, sleeped %.2fms, busy %.2fms\n",
			fps, fps_max, sleep*1000.0, (delta-sleep)*1000.0
		);
		txtblk_edit(txt.stats, print);

		txtctx_add(txt.hello);
		txtctx_add(txt.stats);
		txtctx_add(txt.tea);

		txtctx_add(txt.al);
		txtctx_add(txt.ac);
		txtctx_add(txt.ar);

		engine_tick();

		sleep = (1.0f/(float)fps_max) - (glfwGetTime()-now);
		if(sleep > 0.0) {
			usleep(980*1000*sleep);
		}
		frames++;
	}

	engine_destroy();

	return 0;
}
