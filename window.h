#pragma once

#include <GLFW/glfw3.h>

extern GLFWwindow* window;

extern int view_width;
extern int view_height;

bool window_init();
void window_deinit();
