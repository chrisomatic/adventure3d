#pragma once

typedef struct
{
    Vector3f color;
    float ambient_intensity;
    Vector3f direction;
    float diffuse_intensity;
} Light;

extern Light light;
