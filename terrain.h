#pragma once

void terrain_build(const char* heightmap);
void terrain_get_stats(float x, float z, float* height, Vector3f* ret_norm);
void terrain_render();
