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
	//gfx_init();

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
	gfx_tick();
	glfwPollEvents();

}

