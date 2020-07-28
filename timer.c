#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include "timer.h"

void timer_begin(Timer* timer)
{
    timer->time_start = glfwGetTime();
    timer->time_last = timer->time_start;
}

double timer_get_time()
{
    return glfwGetTime();
}

void timer_set_fps(Timer* timer, float fps)
{
    timer->fps = fps;
    timer->spf = 1.0f / fps;
}

void timer_wait_for_frame(Timer* timer)
{
    while(glfwGetTime() < timer->time_last + timer->spf)
        usleep(100);
}

void timer_wait(Timer* timer, float fps)
{
    float spf = 1.0f / fps;
    while(glfwGetTime() < timer->time_last + spf)
        usleep(100);
}

double timer_get_elapsed(Timer* timer)
{
    double time_curr = glfwGetTime();
    return time_curr - timer->time_start;
}

void timer_inc_frame(Timer* timer)
{
    timer->time_last += timer->spf;
}
