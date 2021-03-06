#pragma once

#define PI        3.14159265358f
#define PI_OVER_2 1.57079632679f

#define RAD(x) (((x) * PI) / 180.0f)
#define DEG(x) (((x) * 180.0f) / PI)
#define ABS(x) ((x) < 0 ? -1*(x) : (x))

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define SQ(x) ((x)*(x))

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

typedef struct
{
    float m[4][4];
} Matrix4f;

typedef struct
{
    float x,y;
} Vector2f;

typedef struct
{
    float x,y,z;
} Vector3f;

typedef struct
{
    float x,y,z,w;
} Vector4f;

typedef struct
{
    float x,y,z,w;
} Quaternion;

typedef struct
{
    Vector3f position;
    Vector2f tex_coord;
    Vector3f normal;
} Vertex;

extern const Matrix4f identity_m4f;

// vector
float magnitude_v3f(Vector3f* v);
float dot_product_v3f(Vector3f* a, Vector3f* b);
void copy_v3f(Vector3f* d, Vector3f* s);
void normalize_v3f(Vector3f* v);
void cross_v3f(Vector3f a, Vector3f b, Vector3f* ret);
void rotate_v3f(float angle, const Vector3f axis, Vector3f* v);
void get_normal_v3f(Vector3f a, Vector3f b, Vector3f c, Vector3f* norm);
void calc_vertex_normals(const unsigned int* indices, unsigned int index_count, Vertex* vertices, unsigned int vertex_count);

// matrix
void dot_product_m4f(Matrix4f a, Matrix4f b, Matrix4f* result);
void print_m4f(const char* title, Matrix4f w);

// other
float barry_centric(Vector3f p1, Vector3f p2, Vector3f p3, Vector2f pos);
