#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "util.h"
#include "math3d.h"
#include "phys.h"

bool phys_collision_line_sphere(Vector3f p1, Vector3f p2, Vector3f s, float r)
{
    // p1 and p2 are points on the line
    // s is the center of sphere
    // r is radius of sphere

    // a = (x2 - x1)2 + (y2 - y1)2 + (z2 - z1)2
    // b = 2[ (x2 - x1) (x1 - x3) + (y2 - y1) (y1 - y3) + (z2 - z1) (z1 - z3) ]
    // c = x32 + y32 + z32 + x12 + y12 + z12 - 2[x3 x1 + y3 y1 + z3 z1] - r2

    float a = SQ(p2.x - p1.x) + SQ(p2.y - p1.y) + SQ(p2.z - p1.z);
    float b = 2.0f*((p2.x - p1.x) * (p1.x - s.x) + (p2.y - p1.y) * (p1.y - s.y) + (p2.z - p1.z) * (p1.z - s.z));
    float c = SQ(s.x) + SQ(s.y) + SQ(s.z) + SQ(p1.x) + SQ(p1.y) + SQ(p1.z) - 2.0f * (s.x*p1.x + s.y*p1.y + s.z*p1.z) - SQ(r);

    float q = b*b-4.0f*a*c;
    printf("Q: %f\n", q);

    return (q >= 0);
}
