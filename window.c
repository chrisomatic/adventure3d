#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "settings.h"
#include "math3d.h"
#include "window.h"
#include "camera.h"
#include "player.h"
#include "lighting.h"
#include "mesh.h"

GLFWwindow* window;

int view_width = STARTING_VIEW_WIDTH;
int view_height = STARTING_VIEW_HEIGHT;

static void window_size_callback(GLFWwindow* window, int window_width, int window_height);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods);

bool window_init()
{
    printf("Initializing GLFW.\n");

    if(!glfwInit())
    {
        fprintf(stderr,"Failed to init GLFW!\n");
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Want to use OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    window = glfwCreateWindow(view_width,view_height,"Adventure",NULL,NULL);

    if(window == NULL)
    {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowAspectRatio(window,ASPECT_NUM,ASPECT_DEM);
    glfwSetWindowSizeCallback(window,window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);

    printf("Initializing GLEW.\n");

    // GLEW
    glewExperimental = 1;
    if(glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return false;
    }

    return true;
}

void window_deinit()
{
    glfwTerminate();
}

static void window_size_callback(GLFWwindow* window, int window_width, int window_height)
{
    printf("Window: W %d, H %d\n",window_width,window_height);

    view_height = window_height;
    view_width  = ASPECT_RATIO * window_height;

    int start_x = (window_width + view_width) / 2.0f - view_width;
    int start_y = (window_height + view_height) / 2.0f - view_height;

    glViewport(start_x,start_y,view_width,view_height);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera_update_angle(xpos, ypos);
}

static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    float max_vel = 0.5f;

    if(action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch(key)
        {
            case GLFW_KEY_W:
                player.key_w_down = true;
                break;
            case GLFW_KEY_S:
                player.key_s_down = true;
                break;
            case GLFW_KEY_A:
                player.key_a_down = true;
                break;
            case GLFW_KEY_D:
                player.key_d_down = true;
                break;
            case GLFW_KEY_SPACE:
                player.key_space = true;
                break;
            case GLFW_KEY_TAB:
                show_wireframe = !show_wireframe;
                printf("Wireframe: %d\n",show_wireframe);
                break;
            case GLFW_KEY_M:
                // toggle camera mode
                if(camera.mode == CAMERA_MODE_FREEFORM)
                    camera.mode = CAMERA_MODE_LOCKED_TO_PLAYER;
                else
                    camera.mode = CAMERA_MODE_FREEFORM;
                printf("Camera mode changed to %d\n",camera.mode);
                break;
            case GLFW_KEY_UP:
                light.ambient_intensity += 0.05f;
                break;
            case GLFW_KEY_DOWN:
                light.ambient_intensity -= 0.05f;
                break;
        }
    }
    else if(action == GLFW_RELEASE)
    {
        switch(key)
        {
            case GLFW_KEY_W:
                player.key_w_down = false;
                break;
            case GLFW_KEY_S:
                player.key_s_down = false;
                break;
            case GLFW_KEY_A:
                player.key_a_down = false;
                break;
            case GLFW_KEY_D:
                player.key_d_down = false;
                break;
            case GLFW_KEY_SPACE:
                player.key_space = false;
                break;
        }
    }

    Vector3f target_velocity = {0};

    camera.velocity.x = 0.0f;
    camera.velocity.z = 0.0f;
    if(!player.is_in_air)
    {
        camera.velocity.y = 0.0f;
    }

    if(player.key_space && !player.is_in_air)
    {
        player.is_in_air = true;
        camera.velocity.y = 1.0f;
    }

    if(player.key_w_down)
    {
        printf("Moving forward!\n");

        if(camera.mode == CAMERA_MODE_LOCKED_TO_PLAYER)
        {
            copy_v3f(&target_velocity,&camera.target);
            target_velocity.y = 0.0f;
            normalize_v3f(&target_velocity);
        }
        else
            copy_v3f(&target_velocity,&camera.target);

        camera.velocity.x += -max_vel * target_velocity.x;
        camera.velocity.y += -max_vel * target_velocity.y;
        camera.velocity.z += -max_vel * target_velocity.z;
    }
    if(player.key_s_down)
    {
        printf("Moving backwards!\n");

        if(camera.mode == CAMERA_MODE_LOCKED_TO_PLAYER)
        {
            copy_v3f(&target_velocity,&camera.target);
            target_velocity.y = 0.0f;
            normalize_v3f(&target_velocity);
        }
        else
            copy_v3f(&target_velocity,&camera.target);

        camera.velocity.x += +max_vel * target_velocity.x;
        camera.velocity.y += +max_vel * target_velocity.y;
        camera.velocity.z += +max_vel * target_velocity.z;
    }

    if(player.key_a_down)
    {
        printf("Moving left!\n");

        Vector3f left;
        cross_v3f(camera.up, camera.target, &left);
        normalize_v3f(&left);

        camera.velocity.x += -max_vel * left.x;
        camera.velocity.y += -max_vel * left.y;
        camera.velocity.z += -max_vel * left.z;
    }
    if(player.key_d_down)
    {
        printf("Moving right!\n");

        Vector3f right;
        cross_v3f(camera.up, camera.target, &right);
        normalize_v3f(&right);

        camera.velocity.x += +max_vel * right.x;
        camera.velocity.y += +max_vel * right.y;
        camera.velocity.z += +max_vel * right.z;
    }
}
