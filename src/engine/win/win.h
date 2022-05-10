#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "common.h"

int win_init();
void win_destroy();
bool win_should_close(void);
void win_close(void);

GLFWwindow *win_get();

