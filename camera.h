#pragma once

#define CAM_MARGIN 10 

typedef enum
{
    CAMERA_MODE_FREEFORM,
    CAMERA_MODE_LOCKED_TO_PLAYER,
} CameraMode;

typedef struct
{
    double x;
    double y;
} Cursor;

typedef struct {

    Vector3f velocity;
    Vector3f position;
    Vector3f target;
    Vector3f up;

    CameraMode mode;

    float angle_h;
    float angle_v;

    Cursor cursor;

} Camera;

extern Camera camera;

void camera_init();
void camera_update();
void camera_update_angle(float cursor_x, float cursor_y);
void get_camera_transform(Matrix4f* mat);
