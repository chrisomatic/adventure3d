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
#include "timer.h"

// =========================
// Global Vars
// =========================

GLuint vbo,ibo;

Timer game_timer = {0};

bool client_connected = false;
bool is_client = false;
int client_id = -1;

typedef struct
{
    Vector3f position;
    float angle_h;
    float angle_v;
} PlayerInfo;

PlayerInfo player_info[MAX_CLIENTS] = {0};
int num_other_players = 0;

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
            if(argv[i][0] == '-' && argv[i][1] == '-')
            {
                // server
                if(strncmp(argv[i]+2,"server",6) == 0)
                    is_server = true;

                // client
                else if(strncmp(argv[i]+2,"client",6) == 0)
                    is_client = true;
            }
            else
            {
                net_client_set_server_ip(argv[i]);
            }
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

    timer_set_fps(&game_timer,TARGET_FPS);
    timer_begin(&game_timer);

    // main game loop
    for(;;)
    {
        glfwPollEvents();
        if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwWindowShouldClose(window) != 0)
            break;

        simulate();
        render();

        timer_wait_for_frame(&game_timer);
        timer_inc_frame(&game_timer);
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
    {
        net_client_init();
    }

    printf("GL version: %s\n",glGetString(GL_VERSION));
    
    glClearColor(0.15f, 0.15f, 0.15f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void simulate()
{
    world.time += TARGET_SPF;

    //printf("\ntime: %f\n",world.time);

    camera_update();
    player_update();

    Vector3f dir;
    copy_v3f(&dir, &light.direction);
    normalize_v3f(&dir);

    shader_set_float(program, "dl.diffuse_intensity", light.diffuse_intensity);
    shader_set_vec3(program, "dl.direction", dir.x, dir.y, dir.z);

    if(is_client)
    {
        ClientData p =
        {
            {
                player.position.x,
                player.position.y,
                player.position.z
            },
            player.angle_h,
            player.angle_v,
            5.0f,
        };

        int bytes_sent = net_client_send((u8*)&p,sizeof(ClientData));

        // read from server
        for(;;)
        {
            bool data_waiting = net_client_data_waiting();

            if(!data_waiting)
                break;

            Packet srvpkt = {0};
            bool is_latest;
            int recv_bytes = net_client_recv(&srvpkt, &is_latest);

            if(!is_latest)
                continue;

            if(recv_bytes > 0)
            {
                WorldState* ws = (WorldState*)srvpkt.data;
                num_other_players = ws->num_clients - 1;

                printf("Num Other Players: %d.\n",ws->num_clients - 1);

                int player_index = 0;
                for(int i = 0; i < ws->num_clients; ++i)
                {
                    if(i == ws->ignore_id)
                        continue;

                    player_info[player_index].position.x = ws->client_data[i].position.x;
                    player_info[player_index].position.y = ws->client_data[i].position.y;
                    player_info[player_index].position.z = ws->client_data[i].position.z;
                    player_info[player_index].angle_h    = ws->client_data[i].angle_h;
                    player_info[player_index].angle_v    = ws->client_data[i].angle_v;
                    
                    printf("Client%d: P %f %f %f R %f %f\n", i,
                            ws->client_data[i].position.x,
                            ws->client_data[i].position.y,
                            ws->client_data[i].position.z,
                            ws->client_data[i].angle_h,
                            ws->client_data[i].angle_v
                    );

                    player_index++;
                }
            }
        }
    }
}

void render()
{
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4f* _world;
    Matrix4f* _wvp;

    terrain_render();

    glUniform3f(dir_light_location.color, light.color.x, light.color.y, light.color.z);
    glUniform1f(dir_light_location.ambient_intensity, light.ambient_intensity);
    glUniform3f(dir_light_location.direction, light.direction.x, light.direction.y, light.direction.z);
    glUniform1f(dir_light_location.diffuse_intensity, light.diffuse_intensity);

    if(camera.perspective == CAMERA_PERSPECTIVE_THIRD_PERSON || camera.mode == CAMERA_MODE_FREE)
    {
        // render current player
        world_set_scale(1.0f,1.0f,1.0f);
        world_set_rotation(-player.angle_v+90.0f,-player.angle_h+90.0f,0.0f);
        world_set_position(-player.position.x,-player.position.y,-player.position.z);

        _world = get_world_transform();
        _wvp   = get_wvp_transform();

        glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
        glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

        mesh_render(&obj);
    }

    // objects
    for(int i = 0; i < num_other_players; ++i)
    {
        world_set_scale(1.0f,1.0f,1.0f);
        world_set_rotation(-player_info[i].angle_v+90.0f,-player_info[i].angle_h+90.0f,0.0f);
        world_set_position(-player_info[i].position.x,-player_info[i].position.y,-player_info[i].position.z);

        _world = get_world_transform();
        _wvp   = get_wvp_transform();

        glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
        glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

        mesh_render(&obj);
    }

    sky_render();

    glfwSwapBuffers(window);
}

