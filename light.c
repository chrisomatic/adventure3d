#include "math3d.h"
#include "light.h"

DirectionalLight sunlight;

void light_init()
{
    sunlight.base.color.x = 1.0f;
    sunlight.base.color.y = 1.0f;
    sunlight.base.color.z = 1.0f;

    sunlight.base.ambient_intensity = 0.40;
    sunlight.base.diffuse_intensity = 0.50;

    sunlight.direction.x = 1.00f;
    sunlight.direction.y = 1.00f;
    sunlight.direction.z = 1.00f;
}

