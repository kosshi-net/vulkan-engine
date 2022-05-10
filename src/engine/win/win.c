#include "win/win.h"
#include "common.h"
#include "event/event.h"

static GLFWwindow *win;

static void win_resize_callback(GLFWwindow *window, int w, int h)
{
	event_fire(EVENT_WIN_RESIZE, NULL);
}

int win_init(void)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);
	win = glfwCreateWindow(800,600,"Teapots",NULL,NULL);
	glfwSetFramebufferSizeCallback(win, win_resize_callback);
	
	return 0;
}

void win_destroy(void)
{
	glfwDestroyWindow(win);
}

bool win_should_close(void)
{
	return glfwWindowShouldClose(win);
}

GLFWwindow * win_get(void)
{
	return win;
}
