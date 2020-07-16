#include "math3d.h"
#include "lighting.h"

Light light;

void lighting_init()
{
    light.color.x = 1.0f;
    light.color.y = 1.0f;
    light.color.z = 1.0f;

    light.ambient_intensity = 0.40;
    light.diffuse_intensity = 0.30;

    light.direction.x = 1.00f;
    light.direction.y = 1.00f;
    light.direction.z = 1.00f;
}

