#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "math3d.h"
#include "settings.h"
#include "camera.h"
#include "transform.h"

static Matrix4f scale_trans            = {0};
static Matrix4f rotation_trans         = {0};
static Matrix4f translate_trans        = {0};
static Matrix4f perspective_trans      = {0};
static Matrix4f camera_translate_trans = {0};
static Matrix4f camera_rotate_trans    = {0};

static Matrix4f world_trans            = {0};
static Matrix4f wvp_trans              = {0};
static Matrix4f vp_trans               = {0};

World world = {0};

static void get_scale_transform(Matrix4f* mat)
{
    memset(mat,0,sizeof(Matrix4f));

    mat->m[0][0] = world.scale.x;
    mat->m[1][1] = world.scale.y;
    mat->m[2][2] = world.scale.z;
    mat->m[3][3] = 1.0f;
}

static void get_rotation_transform(Matrix4f* mat)
{
    Matrix4f rx = {0};
    Matrix4f ry = {0};
    Matrix4f rz = {0};

    const float x = RAD(world.rotation.x);
    const float y = RAD(world.rotation.y);
    const float z = RAD(world.rotation.z);

    rx.m[0][0] = 1.0f;
    rx.m[1][1] = cosf(x);
    rx.m[1][2] = -sinf(x);
    rx.m[2][1] = sinf(x);
    rx.m[2][2] = cosf(x);
    rx.m[3][3] = 1.0f;

    ry.m[0][0] = cosf(y);
    ry.m[0][2] = -sinf(y); 
    ry.m[1][1] = 1.0f;  
    ry.m[2][0] = sinf(y);
    ry.m[2][2] = cosf(y);
    ry.m[3][3] = 1.0f;

    rz.m[0][0] = cosf(z);
    rz.m[0][1] = -sinf(z);
    rz.m[1][0] = sinf(z);
    rz.m[1][1] = cosf(z);
    rz.m[2][2] = 1.0f;
    rz.m[3][3] = 1.0f;

    memset(mat,0,sizeof(Matrix4f));

    dot_product_m4f(identity_m4f,rz,mat);
    dot_product_m4f(*mat,ry,mat);
    dot_product_m4f(*mat,rx,mat);
}

static void get_translate_transform(Matrix4f* mat, Vector3f position)
{
    memset(mat,0,sizeof(Matrix4f));

    mat->m[0][0] = 1.0f;
    mat->m[0][3] = position.x;
    mat->m[1][1] = 1.0f;
    mat->m[1][3] = position.y;
    mat->m[2][2] = 1.0f;
    mat->m[2][3] = position.z;
    mat->m[3][3] = 1.0f;
}

static void get_perspective_transform(Matrix4f* mat)
{
    const float ar           = view_width/view_height;
    const float z_near       = Z_NEAR;
    const float z_far        = Z_FAR;
    const float z_range      = z_near - z_far;
    const float tan_half_fov = tanf(RAD(FOV / 2.0f));

    memset(mat,0,sizeof(Matrix4f));

    mat->m[0][0] = 1.0f / (tan_half_fov * ar);
    mat->m[1][1] = 1.0f / tan_half_fov;
    mat->m[2][2] = (-z_near - z_far) / z_range;
    mat->m[2][3] = 2.0f * z_far * z_near / z_range;
    mat->m[3][2] = 1.0f;
}

Matrix4f* get_world_transform()
{
    get_scale_transform(&scale_trans);
    get_rotation_transform(&rotation_trans);
    get_translate_transform(&translate_trans, world.position);

    memset(&world_trans,0,sizeof(Matrix4f));

    dot_product_m4f(identity_m4f,  translate_trans, &world_trans);
    dot_product_m4f(world_trans,   rotation_trans,  &world_trans);
    dot_product_m4f(world_trans,   scale_trans,     &world_trans);

    return &world_trans;
}

Matrix4f* get_wvp_transform()
{
    get_scale_transform(&scale_trans);
    get_rotation_transform(&rotation_trans);
    get_translate_transform(&translate_trans, world.position);
    get_perspective_transform(&perspective_trans);
    get_translate_transform(&camera_translate_trans, camera.position);
    get_camera_transform(&camera_rotate_trans);

    memset(&wvp_trans,0,sizeof(Matrix4f));

    dot_product_m4f(identity_m4f, perspective_trans,    &wvp_trans);
    dot_product_m4f(wvp_trans,    camera_rotate_trans,    &wvp_trans);
    dot_product_m4f(wvp_trans,    camera_translate_trans, &wvp_trans);
    dot_product_m4f(wvp_trans,    translate_trans,        &wvp_trans);
    dot_product_m4f(wvp_trans,    rotation_trans,         &wvp_trans);
    dot_product_m4f(wvp_trans,    scale_trans,            &wvp_trans);

    return &wvp_trans;
}

Matrix4f* get_vp_transform()
{
    get_perspective_transform(&perspective_trans);
    get_translate_transform(&camera_translate_trans, camera.position);
    get_camera_transform(&camera_rotate_trans);

    memset(&vp_trans,0,sizeof(Matrix4f));

    dot_product_m4f(identity_m4f, perspective_trans,     &vp_trans);
    dot_product_m4f(vp_trans,     camera_rotate_trans,    &vp_trans);
    dot_product_m4f(vp_trans,     camera_translate_trans, &vp_trans);

    return &vp_trans;
}

void transform_world_init()
{
    world.scale.x = 1.0f;
    world.scale.y = 1.0f;
    world.scale.z = 1.0f;
}

void world_set_position(float x, float y, float z)
{
    world.position.x = x;
    world.position.y = y;
    world.position.z = z;
}
void world_set_rotation(float x, float y, float z)
{
    world.rotation.x = x;
    world.rotation.y = y;
    world.rotation.z = z;
}
void world_set_scale(float x, float y, float z)
{
    world.scale.x = x;
    world.scale.y = y;
    world.scale.z = z;
}
