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
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

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
    glfwSetMouseButtonCallback(window, mouse_button_callback);

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
    int mode = glfwGetInputMode(window,GLFW_CURSOR);
    if(mode == GLFW_CURSOR_DISABLED)
        camera_update_angle(xpos, ypos);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        int mode = glfwGetInputMode(window,GLFW_CURSOR);
        if(mode == GLFW_CURSOR_NORMAL)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    if(action == GLFW_PRESS)
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
            case GLFW_KEY_LEFT_SHIFT:
                player.key_shift = true;
                break;
            case GLFW_KEY_TAB:
                show_wireframe = !show_wireframe;
                printf("Wireframe: %d\n",show_wireframe);
                break;
            case GLFW_KEY_M:
                // toggle camera mode
                if(camera.mode == CAMERA_MODE_FREE)
                {
                    camera.mode = CAMERA_MODE_FOLLOW_PLAYER;
                    camera_move_to_player();
                }
                else
                    camera.mode = CAMERA_MODE_FREE;

                printf("Camera mode changed to %d\n",camera.mode);
                break;
            case GLFW_KEY_R:
                // toggle camera perspective
                if(camera.perspective == CAMERA_PERSPECTIVE_FIRST_PERSON)
                    camera.perspective = CAMERA_PERSPECTIVE_THIRD_PERSON;
                else
                    camera.perspective = CAMERA_PERSPECTIVE_FIRST_PERSON;

                printf("Camera perspective changed to %d\n",camera.perspective);
                break;
            case GLFW_KEY_UP:
                light.ambient_intensity += 0.05f;
                break;
            case GLFW_KEY_DOWN:
                light.ambient_intensity -= 0.05f;
                break;
            case GLFW_KEY_ESCAPE:
            {
                int mode = glfwGetInputMode(window,GLFW_CURSOR);
                if(mode == GLFW_CURSOR_DISABLED)
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                else
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }   break;
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
            case GLFW_KEY_LEFT_SHIFT:
                player.key_shift = false;
                break;
        }
    }
    //camera_update();
}
