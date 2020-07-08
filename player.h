#pragma once

typedef struct
{
    bool key_w_down;
    bool key_s_down;
    bool key_a_down;
    bool key_d_down;
    bool key_space;
    bool key_shift;
    bool is_in_air;
} Player;

extern Player player;
