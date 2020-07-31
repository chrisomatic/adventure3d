#pragma once

typedef struct
{
    float  fps;
    float  spf;
    double time_start;
    double time_last;
} Timer;

void timer_begin(Timer* timer);
void timer_set_fps(Timer* timer, float fps);
void timer_wait_for_frame(Timer* timer);
void timer_wait(Timer* timer, float fps);
void timer_inc_frame(Timer* timer);

double timer_get_elapsed(Timer* timer);
void timer_delay_us(int us);
double timer_get_time();
