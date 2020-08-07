#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "math3d.h"
#include "settings.h"
#include "terrain.h"
#include "player.h"

#include "camera.h"

#define ACCEL_DUE_TO_GRAVITY -9.8f
#define MAX_CAMERA_ADJUSTMENT 0.4f

Camera camera;

static float terrain_height;

//
// Prototypes
//

static void camera_update_accel();
static void camera_update_velocity();
static void camera_update_velocity2();
static void camera_update_position();
static void camera_update_perspective();
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

    camera.player_offset.x = 0.0f;
    camera.player_offset.y = 0.0f;
    camera.player_offset.z = 0.0f;

    camera.mode        = CAMERA_MODE_FOLLOW_PLAYER;
    camera.perspective = CAMERA_PERSPECTIVE_FIRST_PERSON;

    camera.perspective_transition = 0.0f;
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

void camera_update()
{
#if 0
    camera_update_accel();
    camera_update_velocity();
#else
    camera_update_velocity2();
#endif

    camera_update_position();
    camera_update_perspective();
    camera_update_rotations();
}

void camera_move_to_player()
{
    camera.velocity.x = player.velocity.x;
    camera.velocity.y = player.velocity.y;
    camera.velocity.z = player.velocity.z;

    camera.position.x = player.position.x;
    camera.position.y = player.position.y;
    camera.position.z = player.position.z;

    camera.target.x = player.target.x;
    camera.target.y = player.target.y;
    camera.target.z = player.target.z;

    camera.up.x = player.up.x;
    camera.up.y = player.up.y;
    camera.up.z = player.up.z;

    camera.angle_h = player.angle_h;
    camera.angle_v = player.angle_v;
}


//
// Static functions
//

static void get_user_force(Vector3f* user_force)
{
    Vector3f target_dir = {0};

    if(player.key_w_down)
    {
        copy_v3f(&target_dir,&camera.target);

        if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
        {
            target_dir.y = 0.0f;
            normalize_v3f(&target_dir);
        }

        user_force->x += -1*player.accel_factor*target_dir.x;
        user_force->y += -1*player.accel_factor*target_dir.y;
        user_force->z += -1*player.accel_factor*target_dir.z;
    }

    if(player.key_s_down)
    {
        copy_v3f(&target_dir,&camera.target);

        if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
        {
            target_dir.y = 0.0f;
            normalize_v3f(&target_dir);
        }

        user_force->x += player.accel_factor*target_dir.x;
        user_force->y += player.accel_factor*target_dir.y;
        user_force->z += player.accel_factor*target_dir.z;
    }

    if(player.key_a_down)
    {
        cross_v3f(camera.up, camera.target, &target_dir);
        normalize_v3f(&target_dir);

        user_force->x += -1*player.accel_factor*target_dir.x;
        user_force->y += -1*player.accel_factor*target_dir.y;
        user_force->z += -1*player.accel_factor*target_dir.z;
    }

    if(player.key_d_down)
    {
        cross_v3f(camera.up, camera.target, &target_dir);
        normalize_v3f(&target_dir);

        user_force->x += player.accel_factor*target_dir.x;
        user_force->y += player.accel_factor*target_dir.y;
        user_force->z += player.accel_factor*target_dir.z;
    }
}

static void camera_update_accel()
{
    // Sum up all force vectors.
    // 1. Gravity
    // 2. User input force
    // 3. Friction
    // 4. Friction (Air)

    Vector3f gravity    = {0};
    Vector3f friction   = {0};
    Vector3f user_force = {0};

    float coeff_friction_ground = 0.3f;
    float coeff_friction_air = 0.1f;

    if(player.is_in_air)
    {
        gravity.y = ACCEL_DUE_TO_GRAVITY;

        friction.x = -coeff_friction_air*user_force.x;
        friction.y = -coeff_friction_air*user_force.y;
        friction.z = -coeff_friction_air*user_force.z;
    }
    else
    {
        terrain_height = 0.0f;
        Vector3f norm = {0};

        terrain_get_stats(
                camera.position.x, 
                camera.position.z,
                &terrain_height,
                &norm
        );

        float slope = norm.y / sqrt(norm.x*norm.x + norm.z*norm.z);
        float angle = atan(slope);// + PI_OVER_2;
        float gravity_parallel = ACCEL_DUE_TO_GRAVITY*cos(angle);

        //printf("Height: %f, Normal: %f %f %f, Angle: %f deg, Grav: %f\n",terrain_height, ret_norm.x, ret_norm.y, ret_norm.z, 90 + DEG(angle), gravity_parallel);
        printf("Grav: %f %f %f\n", gravity_parallel*norm.x, gravity_parallel*norm.y, gravity_parallel*norm.z);

        gravity.x = gravity_parallel*norm.x;
        gravity.y = gravity_parallel*norm.y; 
        gravity.z = gravity_parallel*norm.z; 

        friction.x = -coeff_friction_ground*user_force.x;
        friction.y = -coeff_friction_ground*user_force.y;
        friction.z = -coeff_friction_ground*user_force.z;
    }

    get_user_force(&user_force);

    memset(&camera.accel, 0, sizeof(Vector3f));

    camera.accel.x += gravity.x;
    camera.accel.y += gravity.y;
    camera.accel.z += gravity.z;

    camera.accel.x += user_force.x;
    camera.accel.y += user_force.y;
    camera.accel.z += user_force.z;

    camera.accel.x += friction.x;
    camera.accel.y += friction.y;
    camera.accel.z += friction.z;

    camera.accel.x *= -1;
    camera.accel.y *= -1;
    camera.accel.z *= -1;
}

static void camera_update_velocity()
{
    camera.velocity.x += camera.accel.x;
    camera.velocity.y += camera.accel.y;
    camera.velocity.z += camera.accel.z;
}

static void camera_update_velocity2()
{

    Vector3f norm = {0};

    if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
    {
        terrain_height = 0.0f;
        terrain_get_stats(camera.position.x, camera.position.z, &terrain_height, &norm);

        float margin_of_error = 0.05f;

        if(camera.position.y > player.height + terrain_height)
        {
            // gravity
            camera.velocity.y -= 0.02;
            if(camera.position.y > player.height + terrain_height + margin_of_error)
                player.is_in_air = true;
        }
        else
        {
            camera.velocity.y = 0.0f;
            player.is_in_air = false;
            player.jumped = false;
        }
    }
    else if(camera.mode == CAMERA_MODE_FREE)
    {
        if(player.is_in_air)
            player.is_in_air = false;
    }

    if(player.is_in_air)
        return;

    float accel           = 0.167f; // 1 meter per second
    float max_vel         = 0.175f;
    float friction_factor = 0.008f;

    if(player.key_shift)
    {
        accel   *= 2.0f;
        max_vel *= 2.0f;
    }

    if(camera.mode == CAMERA_MODE_FREE)
    {
        accel   *= 2.0f;
        max_vel *= 2.0f;
    }


    Vector3f target_dir = {camera.target.x, camera.target.y, camera.target.z};

    if(!player.jumped && player.key_space)
    {
        // jump
        player.jumped = true;
        camera.velocity.y += 0.4f;
    }

    if(player.key_w_down)
    {
        if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
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
        if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
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

    // friction
    if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
    {
        // handle slope
        Vector3f dir = {camera.target.x, 0.0f, camera.target.z};

        float dot = dot_product_v3f(&norm, &dir);
        float mag_norm = magnitude_v3f(&norm);
        float mag_targ = magnitude_v3f(&dir);

        float angle = acos(dot/(mag_norm*mag_targ));
        double factor = angle / PI_OVER_2;

        //printf("angle: %f, factor: %f\n",angle,factor);

        if(factor < 10.0f)
        {
            //camera.velocity.x *= factor;
            //camera.velocity.y *= factor;
            //camera.velocity.z *= factor;
        }

        // ground friction
        if(ABS(camera.velocity.x) > 0.0f || ABS(camera.velocity.z) > 0.0f)
        {
            Vector3f friction = {-camera.velocity.x, 0.0f, -camera.velocity.z};

            normalize_v3f(&friction);
            friction.x *= friction_factor;
            friction.z *= friction_factor;

            camera.velocity.x += friction.x;
            camera.velocity.z += friction.z;
        }
    }
    else
    {
        // air friction
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

    float velocity_magnitude;

    if(camera.mode == CAMERA_MODE_FREE)
    {
        velocity_magnitude = magnitude_v3f(&camera.velocity);
    }
    else
    {
        velocity_magnitude = 
            sqrt(camera.velocity.x*camera.velocity.x + 
                 camera.velocity.z*camera.velocity.z);
    }

    float margin_of_error = 0.0166f;

    if(velocity_magnitude > max_vel)
    {
        // set velocity to max
        normalize_v3f(&camera.velocity);

        camera.velocity.x *= max_vel;
        if(camera.mode == CAMERA_MODE_FREE)
            camera.velocity.y *= max_vel;
        camera.velocity.z *= max_vel;
    }
    else if(ABS(velocity_magnitude) < margin_of_error)
    {
        camera.velocity.x = 0.0f;
        if(camera.mode == CAMERA_MODE_FREE)
            camera.velocity.y = 0.0f;
        camera.velocity.z = 0.0f;
    }
}

static void camera_update_position()
{
    camera.position_target.x = camera.position.x + camera.velocity.x;
    camera.position_target.y = camera.position.y + camera.velocity.y;
    camera.position_target.z = camera.position.z + camera.velocity.z;

    if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
    {
        if(camera.position.y < player.height + terrain_height)
            camera.position_target.y = player.height + terrain_height;
    }

    if(camera.position.x != camera.position_target.x)
        camera.position.x = camera.position_target.x;

    if(camera.position.y != camera.position_target.y)
        camera.position.y = camera.position_target.y;

    if(camera.position.z != camera.position_target.z)
        camera.position.z = camera.position_target.z;
}

static void camera_update_perspective()
{
    if(camera.perspective == CAMERA_PERSPECTIVE_THIRD_PERSON)
    {
        if(camera.perspective_transition < 1.0f)
        {
            camera.perspective_transition += 0.1f;
            if(camera.perspective_transition > 1.0f)
                camera.perspective_transition = 1.0f;
        }

    }
    else
    {
        if(camera.perspective_transition > 0.0f)
        {
            camera.perspective_transition -= 0.1f;
            if(camera.perspective_transition < 0.0f)
                camera.perspective_transition = 0.0f;
        }
    }

    camera.player_offset.x = camera.perspective_transition*(10.0f*camera.target.x);
    camera.player_offset.y = camera.perspective_transition*(10.0f*camera.target.y + 6.0f);
    camera.player_offset.z = camera.perspective_transition*(10.0f*camera.target.z);

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
