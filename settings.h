#pragma once

#include "menu.h"

#define STARTING_VIEW_WIDTH   1366
#define STARTING_VIEW_HEIGHT  768

#define ASPECT_NUM 16.0f
#define ASPECT_DEM  9.0f
#define ASPECT_RATIO (ASPECT_NUM / ASPECT_DEM)

#define TARGET_FPS     60.0f
#define TARGET_SPF     (1.0f/TARGET_FPS) // seconds per frame

#define FOV       120.0f 
#define Z_NEAR    0.01f
#define Z_FAR   1000.0f

void update_title_screen();
void update_title_screen_pressed();
void update_title_screen_highlights(double xpos, double ypos);

extern int view_width;
extern int view_height;

extern int is_title_screen;
