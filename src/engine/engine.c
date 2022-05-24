#include "engine/engine.h"
#include "event/event.h"
#include "log/log.h"
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
	
	log_critical("Stack trace:");

	for (int i = 0; i < size; i++) {
		log_critical("%s", strings[i]);
	}
}

#else

void print_trace(void)
{
	log_critical("Stack trace unavailable");
	return;
}

#endif

void _engine_crash(const char *msg, const char *file, const char *func, int line)
{
	_log(LOG_CRITICAL, file, func, line, msg);
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
	engine_wait_idle();

	log_info("Destroying renderers");
	event_fire(EVENT_RENDERERS_DESTROY, NULL);

	log_info("Destroying renderer core");
	gfx_destroy();

	log_info("Destroying window");
	win_destroy();
	glfwTerminate();

	log_info("Goodbye!");
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
	if(vk.dev) {
		log_info("Wait idle");
		vkDeviceWaitIdle(vk.dev);
	}
}

