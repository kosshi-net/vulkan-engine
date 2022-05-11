#include "engine/engine.h"
#include "common.h"

#include "win/win.h"
#include "gfx/gfx.h"

#include "gfx/gfx_types.h"


extern struct VkEngine vk;

int engine_init()
{
	if (!glfwInit()) {
		printf("GLFW Init failure");
		return 1;
	}

	win_init();
	gfx_init();

	return 0;
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
