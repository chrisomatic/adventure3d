#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "math3d.h"
#include "settings.h"
#include "terrain.h"
#include "player.h"

#include "camera.h"

Camera camera;

//
// Prototypes
//

static void camera_update_velocity();
static void camera_update_position();
static void camera_update_rotations();

//
// Global functions
//

void get_camera_transform(Matrix4f* mat)
{
    Vector3f n,u,v;

    copy_v3f(&n,&camera.target);
    copy_v3f(&u,&camera.up);

    normalize_v3f(&n);
    cross_v3f(camera.target,u,&u);
    cross_v3f(n,u,&v);

    memset(mat,0,sizeof(Matrix4f));

    mat->m[0][0] = u.x;
    mat->m[0][1] = u.y;
    mat->m[0][2] = u.z;
    mat->m[1][0] = v.x;
    mat->m[1][1] = v.y;
    mat->m[1][2] = v.z;
    mat->m[2][0] = n.x;
    mat->m[2][1] = n.y;
    mat->m[2][2] = n.z;
    mat->m[3][3] = 1.0f;
}

void camera_init()
{
    memset(&camera,0,sizeof(Camera));

    camera.position.y = player.height;
    camera.target.z   = 1.0f;
    camera.up.y       = 1.0f;

    Vector3f h_target = {
        camera.target.x,
        0.0f,
        camera.target.z
    };

    normalize_v3f(&h_target);

    if(h_target.z >= 0.0f)
    {
        if(h_target.x >= 0.0f)
            camera.angle_h = 360.0f - DEG(asin(h_target.z));
        else
            camera.angle_h = 180.0f + DEG(asin(h_target.z));
    }
    else
    {
        if(h_target.x >= 0.0f)
            camera.angle_h = DEG(asin(-h_target.z));
        else
            camera.angle_h = 180.0f - DEG(asin(-h_target.z));
    }

    camera.angle_v = -DEG(asin(camera.target.y));

    camera.cursor.x = view_width / 2.0f;
    camera.cursor.y = view_height / 2.0f;

    camera.mode = CAMERA_MODE_LOCKED_TO_PLAYER;
}


void camera_update_angle(float cursor_x, float cursor_y)
{
    int delta_x = camera.cursor.x - cursor_x;
    int delta_y = camera.cursor.y - cursor_y;

    camera.cursor.x = cursor_x;
    camera.cursor.y = cursor_y;

    camera.angle_h += (float)delta_x / 16.0f;
    camera.angle_v += (float)delta_y / 16.0f;

    if(camera.angle_h > 360)
        camera.angle_h -= 360.0f;
    else if(camera.angle_h < 0)
        camera.angle_h += 360.f;

    if(camera.angle_v > 90)
        camera.angle_v = 90.0f;
    else if(camera.angle_v < -90)
        camera.angle_v = -90.0f;

   //printf("Angle: H %f, V %f\n",camera.angle_h,camera.angle_v);
}

//
// Static functions
//

void camera_update()
{
    camera_update_velocity();
    camera_update_position();
    camera_update_rotations();
}

static void camera_update_velocity()
{
    if(camera.mode == CAMERA_MODE_LOCKED_TO_PLAYER && player.is_in_air)
        return; // can't adjust velocity while in air

    if(camera.mode == CAMERA_MODE_FREEFORM)
        if(player.is_in_air)
            player.is_in_air = false;


    float accel    = 0.1f;
    float max_vel  = 0.8f;
    float friction_factor = 0.02f;

    if(player.key_shift)
    {
        accel   *= 2.0f;
        max_vel *= 2.0f;
    }

    if(camera.mode == CAMERA_MODE_FREEFORM)
    {
        accel   *= 2.0f;
        max_vel *= 2.0f;
    }

    Vector3f target_dir = {camera.target.x, camera.target.y, camera.target.z};

    if(player.key_space)
    {
        // jump
        camera.velocity.y = 1.5f;
    }

    if(player.key_w_down)
    {
        if(camera.mode == CAMERA_MODE_LOCKED_TO_PLAYER)
        {
            copy_v3f(&target_dir,&camera.target);
            target_dir.y = 0.0f;
            normalize_v3f(&target_dir);
        }
        else
            copy_v3f(&target_dir,&camera.target);

        camera.velocity.x += -accel * target_dir.x;
        camera.velocity.y += -accel * target_dir.y;
        camera.velocity.z += -accel * target_dir.z;
    }
    if(player.key_s_down)
    {
        if(camera.mode == CAMERA_MODE_LOCKED_TO_PLAYER)
        {
            copy_v3f(&target_dir,&camera.target);
            target_dir.y = 0.0f;
            normalize_v3f(&target_dir);
        }
        else
            copy_v3f(&target_dir,&camera.target);

        camera.velocity.x += +accel * target_dir.x;
        camera.velocity.y += +accel * target_dir.y;
        camera.velocity.z += +accel * target_dir.z;
    }

    if(player.key_a_down)
    {
        Vector3f left;
        cross_v3f(camera.up, camera.target, &left);
        normalize_v3f(&left);

        camera.velocity.x += -accel * left.x;
        camera.velocity.y += -accel * left.y;
        camera.velocity.z += -accel * left.z;
    }
    if(player.key_d_down)
    {
        Vector3f right;
        cross_v3f(camera.up, camera.target, &right);
        normalize_v3f(&right);

        camera.velocity.x += +accel * right.x;
        camera.velocity.y += +accel * right.y;
        camera.velocity.z += +accel * right.z;
    }

    if(!player.is_in_air)
    {
        // friction
        if(ABS(camera.velocity.x) > 0.0f || ABS(camera.velocity.y) > 0.0f || ABS(camera.velocity.z) > 0.0f)
        {
            Vector3f friction = {-camera.velocity.x, -camera.velocity.y, -camera.velocity.z};

            normalize_v3f(&friction);
            friction.x *= friction_factor;
            friction.y *= friction_factor;
            friction.z *= friction_factor;

            camera.velocity.x += friction.x;
            camera.velocity.y += friction.y;
            camera.velocity.z += friction.z;
        }
    }

    float velocity_magnitude = sqrt(camera.velocity.x*camera.velocity.x + 
                                    //camera.velocity.y*camera.velocity.y + 
                                    camera.velocity.z*camera.velocity.z );

    if(velocity_magnitude > max_vel)
    {
        // set velocity to max
        normalize_v3f(&camera.velocity);
        camera.velocity.x *= max_vel;
        //camera.velocity.y *= max_vel;
        camera.velocity.z *= max_vel;
    }
    else if(velocity_magnitude < 0)
    {
        camera.velocity.x = 0;
        camera.velocity.y = 0;
        camera.velocity.z = 0;
    }
}

static void camera_update_position()
{
    camera.position.x += camera.velocity.x;
    camera.position.y += camera.velocity.y;
    camera.position.z += camera.velocity.z;

    // @TODO: Move this code into "player" file
    float terrain_height = terrain_get_height(camera.position.x, camera.position.z);

    if(camera.mode == CAMERA_MODE_LOCKED_TO_PLAYER)
    {
        float margin_of_error = 10.0f;
        if(camera.position.y > player.height + terrain_height)
        {
            // gravity
            camera.velocity.y -= 0.05f;

            if(camera.position.y > player.height + terrain_height + margin_of_error)
                player.is_in_air = true;
        }
        else
        {
            camera.position.y = player.height + terrain_height;
            camera.velocity.y = 0.0f;
            player.is_in_air = false;
        }
    }

}

static void camera_update_rotations()
{
    const Vector3f v_axis = {0.0f, 1.0f, 0.0f};

    // Rotate the view vector by the horizontal angle around the vertical axis
    Vector3f view = {1.0f, 0.0f, 0.0f};
    rotate_v3f(camera.angle_h, v_axis, &view);
    normalize_v3f(&view);

    // Rotate the view vector by the vertical angle around the horizontal axis
    Vector3f h_axis = {0};

    cross_v3f(v_axis, view, &h_axis);
    normalize_v3f(&h_axis);
    rotate_v3f(camera.angle_v, h_axis, &view);

    copy_v3f(&camera.target,&view);
    normalize_v3f(&camera.target);

    normalize_v3f(&camera.target);

    //printf("Target: %f %f %f\n", camera.target.x, camera.target.y, camera.target.z);

    cross_v3f(camera.target,h_axis, &camera.up);
    normalize_v3f(&camera.up);

    //printf("Up: %f %f %f\n", camera.up.x, camera.up.y, camera.up.z);

}
