#include "engine/engine.h"
#include "common.h"

#include "win/win.h"
#include "gfx/gfx.h"

#include "gfx/gfx_types.h"

/* Place header-only library implementations here */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


#ifndef NDEBUG
#include "execinfo.h"
void print_trace(void)
{
	void *buffer[10];
	char **strings;
	int size;

	size = backtrace(buffer, LENGTH(buffer));
	strings = backtrace_symbols(buffer, size);
	
	for (int i = 0; i < size; i++) {
		fprintf(stderr, "%s\n", strings[i]);
	}

}
#else
void print_trace(void)
{
	return;
}
#endif

void engine_crash_raw(const char *msg, const char *file, const char *func, int line)
{
	fprintf(stderr,"Crashed!\n%s:%s:%i :: %s\n", file, func, line, msg);
	print_trace();
	exit(1);
}


extern struct VkEngine vk;

void engine_init()
{
	if (!glfwInit()) 
		engine_crash("Can't init GLFW");

	win_init();
	gfx_init();
}

void engine_destroy()
{
	gfx_destroy();
	win_destroy();
	glfwTerminate();
}

void engine_tick()
{
	glfwPollEvents();
}

static struct Frame _frame;
static double       time_last;

struct Frame *frame_begin()
{
	double now = glfwGetTime();
	double delta = now - time_last;
	time_last = now;

	struct Frame *frame = &_frame;
	frame->delta = delta;
	frame->vk    = gfx_frame_get();

	frame->width  = vk.swapchain_extent.width;
	frame->height = vk.swapchain_extent.height;

	return frame;
}

void frame_end(struct Frame *frame)
{
	gfx_frame_submit(frame->vk);
}

void engine_wait_idle(void)
{
	if(vk.dev) vkDeviceWaitIdle(vk.dev);
}

