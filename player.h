#pragma once

typedef struct
{
    float height;
    float mass;

    Vector3f accel;
    Vector3f velocity;
    Vector3f position;
    Vector3f target;
    Vector3f up;

    float angle_h;
    float angle_v;

    bool key_w_down;
    bool key_s_down;
    bool key_a_down;
    bool key_d_down;
    bool key_space;
    bool key_shift;
    bool is_in_air;
    bool jumped;
} Player;

typedef struct
{
    Vector3f position;
    float angle_h;
    float angle_v;
} PlayerPacket;

extern Player player;

void player_init();
void player_update();
