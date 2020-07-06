#pragma once


typedef struct
{
    double time;
    Vector3f scale;
    Vector3f position;
    Vector3f rotation;
} World;

extern World world;

void init_world();

Matrix4f* get_world_transform();
Matrix4f* get_wvp_transform();

void world_set_position(float x, float y, float z);
void world_set_rotation(float x, float y, float z);
void world_set_scale(float x, float y, float z);
