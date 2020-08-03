#pragma once

typedef enum
{
    CAMERA_MODE_FREE,
    CAMERA_MODE_FOLLOW_PLAYER,
} CameraMode;

typedef enum
{
    CAMERA_PERSPECTIVE_FIRST_PERSON,
    CAMERA_PERSPECTIVE_THIRD_PERSON,
} CameraPerspective;

typedef struct
{
    double x;
    double y;
} Cursor;

typedef struct {

    Vector3f accel;
    Vector3f velocity;
    Vector3f position;
    Vector3f target;
    Vector3f up;
    Vector3f player_offset;

    CameraMode mode;
    CameraPerspective perspective;

    float perspective_transition; // 0.0 -> 1.0

    float angle_h;
    float angle_v;

    Cursor cursor;

} Camera;

extern Camera camera;

void camera_init();
void camera_update();
void camera_update_angle(float cursor_x, float cursor_y);
void get_camera_transform(Matrix4f* mat);
void camera_move_to_player();
