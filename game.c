#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "settings.h"
#include "window.h"
#include "shader.h"
#include "texture.h"
#include "math3d.h"
#include "camera.h"
#include "transform.h"
#include "player.h"
#include "lighting.h"
#include "mesh.h"
#include "sky.h"
#include "terrain.h"
#include "net.h"

// =========================
// Global Vars
// =========================

GLuint vbo,ibo;

bool is_client = false;

// =========================
// Function Prototypes
// =========================

void start_server();
void start_game();
void init();
void deinit();
void simulate();
void render();

// =========================
// Main Loop
// =========================

int main(int argc, char* argv[])
{
    bool is_server = false;

    if(argc > 1)
    {
        for(int i = 1; i < argc; ++i)
        {
            // --server
            if(argv[i][0] == '-' && argv[i][1] == '-')
                if(strncmp(argv[i]+2,"server",6) == 0)
                    is_server = true;
                else if(strncmp(argv[i]+2,"client",6) == 0)
                    is_client = true;
        }
    }

    if(is_server)
        start_server();
    else
        start_game();

    return 0;
}

// =========================
// Functions
// =========================

void start_server()
{
    net_server_start();
}

void start_game()
{
    init();

    double lasttime = glfwGetTime();

    // main game loop
    for(;;)
    {
        glfwPollEvents();
        if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwWindowShouldClose(window) != 0)
            break;

        simulate();
        render();

        // wait for next frame
        while(glfwGetTime() < lasttime + TARGET_SPF)
            usleep(100);

        lasttime += TARGET_SPF;
    }

    deinit();

}

void init()
{
    bool success;

    success = window_init();
    if(!success)
    {
        fprintf(stderr,"Failed to initialize window!\n");
        exit(1);
    }

    if(is_client)
        net_client_init();

    printf("GL version: %s\n",glGetString(GL_VERSION));
    
    glClearColor(0.15f, 0.15f, 0.15f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    printf("Loading textures.\n");
    texture_load_all();

    printf("Loading shaders.\n");
    shader_load_all();

    printf("Creating meshes.\n");
    mesh_init_all();

    printf("Building terrain.\n");
    terrain_build("textures/heightmap.png");

    player_init();
    camera_init();
    transform_world_init();
    lighting_init();
    sky_init();

    glUniform1i(sampler, 0);

    // put pointer in center of window
    glfwSetCursorPos(window, camera.cursor.x, camera.cursor.y);
}

void deinit()
{
    glDeleteBuffers(1, &vbo);

    shader_deinit();
    if(is_client)
        net_client_deinit();
    window_deinit();
}

ClientPacket pkt_prior = {0};

void simulate()
{
    world.time += TARGET_SPF;

    //printf("\ntime: %f\n",world.time);

    camera_update();

    Vector3f dir;
    copy_v3f(&dir, &light.direction);
    normalize_v3f(&dir);

    shader_set_float(program, "dl.diffuse_intensity", light.diffuse_intensity);
    shader_set_vec3(program, "dl.direction", dir.x, dir.y, dir.z);

    if(is_client)
    {
        ClientPacket pkt = {
            {
            camera.position.x,
            camera.position.y,
            camera.position.z
            }
        };

        if(memcmp(&pkt,&pkt_prior,sizeof(ClientPacket)) != 0)
        {
            memcpy(&pkt_prior,&pkt,sizeof(ClientPacket));
            net_client_send(&pkt);
        }
    }

    //ServerPacket srvpkt = {0};
    //net_client_recv(&srvpkt);
    //printf("Num Clients: %d.\n",srvpkt.num_clients);
}

void render()
{
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4f* _world;
    Matrix4f* _wvp;

    terrain_render();

    // object
    world_set_scale(1.0f,1.0f,1.0f);
    world_set_rotation(10*world.time,10*world.time,0.0f);
    world_set_position(0.0f,10.0f*sinf(world.time),40.0f); //ABS(50.0f*sinf(world.time)));

    _world = get_world_transform();
    _wvp   = get_wvp_transform();

    //shader_set_mat4(program, "world", _world);
    //shader_set_mat4(program, "wvp", _wvp);

    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

    glUniform3f(dir_light_location.color, light.color.x, light.color.y, light.color.z);
    glUniform1f(dir_light_location.ambient_intensity, light.ambient_intensity);
    glUniform3f(dir_light_location.direction, light.direction.x, light.direction.y, light.direction.z);
    glUniform1f(dir_light_location.diffuse_intensity, light.diffuse_intensity);

    mesh_render(&obj);

    sky_render();

    glfwSwapBuffers(window);
}

