#pragma once

void terrain_build(const char* heightmap);
void terrain_get_stats(float x, float z, float* height, float* angle_xy, float* angle_zy);
void terrain_render();
