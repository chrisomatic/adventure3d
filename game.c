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
#include "light.h"
#include "mesh.h"
#include "sky.h"
#include "terrain.h"
#include "net.h"
#include "text.h"
#include "timer.h"

// =========================
// Global Vars
// =========================

GLuint vbo,ibo;

Timer game_timer = {0};

bool client_connected = false;
bool is_client = false;

typedef struct
{
    Vector3f position;
    float angle_h;
    float angle_v;
    float time_received;
} PlayerDataPoint;

typedef struct
{
    PlayerDataPoint prior;
    PlayerDataPoint current;
    
    double delta_time;
    double time_since_last_packet;
    Vector3f guessed_velocity;
    float guessed_angle_h_vel;
    float guessed_angle_v_vel;
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

    text_init();
    player_init();
    camera_init();
    transform_world_init();
    light_init();
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
    copy_v3f(&dir, &sunlight.direction);
    normalize_v3f(&dir);

    shader_set_float(program, "dl.diffuse_intensity", sunlight.base.diffuse_intensity);
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

                    // copy current point to prior
                    memcpy(&player_info[player_index].prior, &player_info[player_index].current,sizeof(PlayerDataPoint));

                    PlayerDataPoint* c = &player_info[player_index].current;
                    PlayerDataPoint* p = &player_info[player_index].prior;

                    c->time_received = timer_get_time();
                    c->position.x    = ws->client_data[i].position.x;
                    c->position.y    = ws->client_data[i].position.y;
                    c->position.z    = ws->client_data[i].position.z;
                    c->angle_h       = ws->client_data[i].angle_h;
                    c->angle_v       = ws->client_data[i].angle_v;

                    player_info[player_index].time_since_last_packet = 0.0f;

                    // calculate 
                    player_info[player_index].delta_time = 
                        c->time_received - p->time_received;

                    player_info[player_index].guessed_velocity.x = 
                        (c->position.x - p->position.x) / player_info[player_index].delta_time;

                    player_info[player_index].guessed_velocity.y = 
                        (c->position.y - p->position.y) / player_info[player_index].delta_time;

                    player_info[player_index].guessed_velocity.z = 
                        (c->position.z - p->position.z) / player_info[player_index].delta_time;

                    player_info[player_index].guessed_angle_h_vel = 
                        (RAD(c->angle_h) - RAD(p->angle_h)) / player_info[player_index].delta_time;

                    player_info[player_index].guessed_angle_v_vel = 
                        (RAD(c->angle_v) - RAD(p->angle_v)) / player_info[player_index].delta_time;
                    
                    /*
                    printf("Client%d: P %f %f %f R %f %f\n", i,
                            ws->client_data[i].position.x,
                            ws->client_data[i].position.y,
                            ws->client_data[i].position.z,
                            ws->client_data[i].angle_h,
                            ws->client_data[i].angle_v
                    );
                    */

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

    glUniform3f(dir_light_location.color, sunlight.base.color.x, sunlight.base.color.y, sunlight.base.color.z);
    glUniform1f(dir_light_location.ambient_intensity, sunlight.base.ambient_intensity);
    glUniform3f(dir_light_location.direction, sunlight.direction.x, sunlight.direction.y, sunlight.direction.z);
    glUniform1f(dir_light_location.diffuse_intensity, sunlight.base.diffuse_intensity);

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

        float angle_v_interp = player_info[i].time_since_last_packet*(player_info[i].guessed_angle_v_vel);
        float angle_h_interp = player_info[i].time_since_last_packet*(player_info[i].guessed_angle_h_vel);

        world_set_rotation(
            -(player_info[i].current.angle_v + DEG(angle_v_interp))+90.0f,
            -(player_info[i].current.angle_h + DEG(angle_h_interp))+90.0f,
            0.0f
        );

        Vector3f pos_interp = {
            player_info[i].time_since_last_packet*(player_info[i].guessed_velocity.x),
            player_info[i].time_since_last_packet*(player_info[i].guessed_velocity.y),
            player_info[i].time_since_last_packet*(player_info[i].guessed_velocity.z)
        };

        //printf("Interp: P %f %f %f R %f %f\n",pos_interp.x, pos_interp.y, pos_interp.z, angle_h_interp, angle_v_interp);

        world_set_position(
            -(player_info[i].current.position.x + pos_interp.x),
            -(player_info[i].current.position.y + pos_interp.y),
            -(player_info[i].current.position.z + pos_interp.z)
        );

        player_info[i].time_since_last_packet += TARGET_SPF;

        _world = get_world_transform();
        _wvp   = get_wvp_transform();

        glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
        glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

        mesh_render(&obj);
    }

    sky_render();

    text_print(10.0f,10.0f,"Hello!");

    glfwSwapBuffers(window);
}

