#include "engine/engine.h"
#include "input.h"
#include "win/win.h"

static GLFWwindow *window;

static struct {
	bool cursor_locked;
	double cursor_x;
	double cursor_y;
} input = {
	.cursor_locked = false,
};

void input_mouse_callback(
	GLFWwindow *window, 
	int key, int action, int mods
){
	if (action == GLFW_RELEASE) {
		if (key == GLFW_MOUSE_BUTTON_LEFT) {
			input_cursor_lock();
		}
	}
}

bool input_getkey(int key) {
	return glfwGetKey(window, key)==GLFW_PRESS;
}

void input_key_callback( 
	GLFWwindow *window, 
	int key, int scancode, int action, int mods
){

	if (action == GLFW_PRESS) {

		if (key == GLFW_KEY_ESCAPE) 
			input_cursor_unlock();

		if (key == GLFW_KEY_Q) 
			win_close();

		if (key == GLFW_KEY_DELETE) 
			engine_crash("Test crash");
		
	}

	if (action == GLFW_RELEASE) {

	}
}


int  input_init(void)
{
	window = win_get();
	
	glfwSetKeyCallback(window, input_key_callback);
	glfwSetMouseButtonCallback(window, input_mouse_callback);

	return 0;
}

void input_cursor_lock(void)
{
	glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED  );
	input.cursor_locked = true;
}

void input_cursor_unlock(void)
{
	glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_NORMAL  );
	input.cursor_locked = false;
}

void input_cursor_pos( double *x, double *y )
{
	glfwGetCursorPos(window, x, y);
}

void input_cursor_delta(double *x, double *y)
{
	double new_x, new_y;
	input_cursor_pos(&new_x, &new_y);
	
	if (!input.cursor_locked) {
		*x = 0;
		*y = 0;
	} else {
		*x = new_x - input.cursor_x;
		*y = new_y - input.cursor_y;
	}

	input.cursor_x = new_x;
	input.cursor_y = new_y;
}

float* input_vector()
{
	static float vec[3];

	vec[2] = 0;
	vec[0] = 0;
	vec[1] = 0;

	if (!input.cursor_locked) return vec;

	/* forward/backward = ws = z */
	vec[2] +=  1.0*input_getkey(GLFW_KEY_W);
	vec[2] += -1.0*input_getkey(GLFW_KEY_S);

	vec[0] +=  1.0*input_getkey(GLFW_KEY_A);
	vec[0] += -1.0*input_getkey(GLFW_KEY_D);

	vec[1] +=  1.0*input_getkey(GLFW_KEY_LEFT_SHIFT);
	vec[1] += -1.0*input_getkey(GLFW_KEY_SPACE);

	return vec;
}
