#pragma once

#define STARTING_VIEW_WIDTH   1366
#define STARTING_VIEW_HEIGHT  768

#define ASPECT_NUM 16.0f
#define ASPECT_DEM  9.0f
#define ASPECT_RATIO (ASPECT_NUM / ASPECT_DEM)

#define TARGET_FPS     60
#define TARGET_SPF     (1.0f/TARGET_FPS) // seconds per frame

#define FOV      96.0f 
#define Z_NEAR    1.0f
#define Z_FAR   256.0f

extern int view_width;
extern int view_height;
