#include <stdio.h>
#include <string.h>
#include <math.h>

#include "util.h"
#include "math3d.h"

const Matrix4f identity_m4f = {
    .m = {
        {1.0f,0.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f,0.0f},
        {0.0f,0.0f,1.0f,0.0f},
        {0.0f,0.0f,0.0f,1.0f}
    }
};

//
// Quaternion
//

static void conjugate_q(Quaternion q, Quaternion* ret)
{
    ret->x = -q.x;
    ret->y = -q.y;
    ret->z = -q.z;
    ret->w = q.w;
}

static void multiply_q_v3f(Quaternion q, Vector3f v, Quaternion* ret)
{
    ret->w = - (q.x * v.x) - (q.y * v.y) - (q.z * v.z);
    ret->x =   (q.w * v.x) + (q.y * v.z) - (q.z * v.y);
    ret->y =   (q.w * v.y) + (q.z * v.x) - (q.x * v.z);
    ret->z =   (q.w * v.z) + (q.x * v.y) - (q.y * v.x);
}

static void multiply_q(Quaternion a,Quaternion b, Quaternion* ret)
{
    ret->w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z);
    ret->x = (a.x * b.w) + (a.w * b.x) + (a.y * b.z) - (a.z * b.y);
    ret->y = (a.y * b.w) + (a.w * b.y) + (a.z * b.x) - (a.x * b.z);
    ret->z = (a.z * b.w) + (a.w * b.z) + (a.x * b.y) - (a.y * b.x);
}

//
// Vectors
//

float magnitude_v3f(Vector3f* v)
{
    return sqrt(v->x * v->x + v->y*v->y + v->z*v->z);
}

void copy_v3f(Vector3f* d, Vector3f* s)
{
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

void normalize_v3f(Vector3f* v)
{
    float len = magnitude_v3f(v);

    if(len == 0)
    {
        v->x = 0.0f;
        v->y = 0.0f;
        v->z = 0.0f;
        return;
    }

    v->x /= len;
    v->y /= len;
    v->z /= len;
}

void cross_v3f(Vector3f a, Vector3f b, Vector3f* ret)
{
    ret->x = a.y * b.z - a.z * b.y;
    ret->y = a.z * b.x - a.x * b.z;
    ret->z = a.x * b.y - a.y * b.x;
}

void rotate_v3f(float angle, const Vector3f axis, Vector3f* v)
{
    const float sin_half_angle = sinf(RAD(angle/2.0f));
    const float cos_half_angle = cosf(RAD(angle/2.0f));

    const float rx = axis.x * sin_half_angle;
    const float ry = axis.y * sin_half_angle;
    const float rz = axis.z * sin_half_angle;
    const float rw = cos_half_angle;

    Quaternion rotation = {rx, ry, rz, rw};

    Quaternion conj;
    conjugate_q(rotation, &conj);

    Quaternion w;

    multiply_q_v3f(rotation, *v, &w);
    multiply_q(w,conj, &w);

    v->x = w.x;
    v->y = w.y;
    v->z = w.z;
}

float dot_product_v3f(Vector3f* a, Vector3f* b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

void sub_v3f(Vector3f a, Vector3f b, Vector3f* ret)
{
    ret->x = a.x - b.x;
    ret->y = a.y - b.y;
    ret->z = a.z - b.z;
}

void add_v3f(Vector3f a, Vector3f b, Vector3f* ret)
{
    ret->x = a.x + b.x;
    ret->y = a.y + b.y;
    ret->z = a.z + b.z;
}

void get_normal_v3f(Vector3f a, Vector3f b, Vector3f c, Vector3f* norm)
{
    Vector3f ba, ca;

    sub_v3f(b,a,&ba);
    sub_v3f(c,a,&ca);

    cross_v3f(ba, ca, norm);
    normalize_v3f(norm);
}

void calc_vertex_normals(const u32* indices, u32 index_count, Vertex* vertices, u32 vertex_count)
{
    for (int i = 0; i < index_count; i += 3)
    {
        u32 i0 = indices[i];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        Vector3f v1,v2;

        sub_v3f(vertices[i1].position, vertices[i0].position, &v1);
        sub_v3f(vertices[i2].position, vertices[i0].position, &v2);

        Vector3f normal;
        cross_v3f(v1, v2, &normal);
        normalize_v3f(&normal);

        add_v3f(normal, vertices[i0].normal, &vertices[i0].normal);
        add_v3f(normal, vertices[i1].normal, &vertices[i1].normal);
        add_v3f(normal, vertices[i2].normal, &vertices[i2].normal);
    }

    for (int i = 0 ; i < vertex_count ; ++i)
    {
        normalize_v3f(&vertices[i].normal);
    }
}

float barry_centric(Vector3f p1, Vector3f p2, Vector3f p3, Vector2f pos)
{
    float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
    float l1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.y - p3.z)) / det;
    float l2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.y - p3.z)) / det;
    float l3 = 1.0f - l1 - l2;

    return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}


//
// Matrices
//

void dot_product_m4f(Matrix4f a, Matrix4f b, Matrix4f* result)
{
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            result->m[i][j] =
                a.m[i][0] * b.m[0][j] + 
                a.m[i][1] * b.m[1][j] + 
                a.m[i][2] * b.m[2][j] + 
                a.m[i][3] * b.m[3][j];
        }
    }
}

void print_m4f(const char* title, Matrix4f w)
{
    if(title != NULL)
    {
        printf("%s",title);
        printf("\n");
    }
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            printf("%6.2f ", w.m[i][j]);
        }
        printf("\n");
    }
}
