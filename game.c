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
#include "phys.h"
#include "sphere.h"
#include "menu.h"

// =========================
// Global Vars
// =========================

Timer game_timer = {0};

int is_title_screen = true;
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
    char player_name[16];
    PlayerDataPoint prior;
    PlayerDataPoint current;
    
    double delta_time;
    double time_since_last_packet;
    Vector3f guessed_velocity;
    float guessed_angle_h_vel;
    float guessed_angle_v_vel;
    bool highlighted;
} PlayerInfo;

PlayerInfo player_info[MAX_CLIENTS] = {0};
int num_other_players = 0;

MenuItemList title_screen = {0};

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

static void start_local_game()
{
    is_title_screen = false;
    is_client = false;
}

static void join_public_server()
{
    is_title_screen = false;
    is_client = true;
}

static void exit_game()
{
    glfwSetWindowShouldClose(window,1);
}

void update_title_screen()
{
    menu_clear_all(&title_screen);

    float start_x = view_width/2.0f;
    float start_y = view_height / 2.0f - 100.0f;

    menu_add_item2(&title_screen,start_x-80.0f,start_y-100.0f,160.0f,25.0f,"ADVENTURE",0,NULL);

    menu_add_item2(&title_screen,start_x-100.0f,start_y,200.0f,25.0f,"Start Local Game",1, &start_local_game);
    menu_add_item2(&title_screen,start_x-100.0f,start_y+30.0f,200.0f,25.0f,"Join Public Server",1, &join_public_server);
    menu_add_item2(&title_screen,start_x-50.0f,start_y+60.0f,100.0f,25.0f,"Exit",1, &exit_game);
}

void update_title_screen_pressed()
{
    for(int i = 0; i < title_screen.count; ++i)
    {
        if(title_screen.items[i].is_highlighted)
        {
            (*title_screen.items[i].fn)();
        }
    }
}

void update_title_screen_highlights(double xpos, double ypos)
{
    for(int i = 0; i < title_screen.count; ++i)
    {
        title_screen.items[i].is_highlighted = 0;
        if(title_screen.items[i].is_interactive)
        {
            if(xpos >= title_screen.items[i].x && xpos <= title_screen.items[i].x + title_screen.items[i].w &&
               ypos >= title_screen.items[i].y-title_screen.items[i].h && ypos <= title_screen.items[i].y) 
            {
                title_screen.items[i].is_highlighted = 1;
            }
        }
    }
}

Sphere my_sphere = {0};

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
    light_init();
    sky_init();
    text_init();

    sphere_create(3,1.0f,&my_sphere);

    menu_init(6,&title_screen);
}

void deinit()
{
    shader_deinit();
    if(is_client)
        net_client_deinit();
    window_deinit();
}

void simulate()
{
    if(is_title_screen)
        return;

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

                    //strncpy(player_info[player_index].player_name,ws->client_data[i].name,16);

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

    for(int i = 0; i < num_other_players; ++i)
    {
        Vector3f p1,p2;

        p1.x = camera.position.x;
        p1.y = camera.position.y;
        p1.z = camera.position.z;

        float view_distance = 10.0f;

        p2.x = camera.position.x + view_distance*camera.target.x;
        p2.y = camera.position.y + view_distance*camera.target.y;
        p2.z = camera.position.z + view_distance*camera.target.z;

        if(phys_collision_line_sphere(p1,p2,player_info[i].current.position,2.0f))
            player_info[i].highlighted = true;
        else
            player_info[i].highlighted = false;
    }
}

void render()
{
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(is_title_screen)
    {
        menu_render(&title_screen);
    }
    else
    {
        terrain_render();

        if(camera.perspective == CAMERA_PERSPECTIVE_THIRD_PERSON || camera.mode == CAMERA_MODE_FREE)
        {
            Vector3f pos      = {-player.position.x, -player.position.y, -player.position.z};
            Vector3f rotation = {-player.angle_v+90.0f, -player.angle_h+90.0f, 0.0f};
            Vector3f scale    = {1.0f, 1.0f, 1.0f};

            mesh_render(&obj, pos, rotation, scale);
        }

        // objects
        for(int i = 0; i < num_other_players; ++i)
        {
            float angle_v_interp = player_info[i].time_since_last_packet*(player_info[i].guessed_angle_v_vel);
            float angle_h_interp = player_info[i].time_since_last_packet*(player_info[i].guessed_angle_h_vel);

            Vector3f pos_interp = {
                player_info[i].time_since_last_packet*(player_info[i].guessed_velocity.x),
                player_info[i].time_since_last_packet*(player_info[i].guessed_velocity.y),
                player_info[i].time_since_last_packet*(player_info[i].guessed_velocity.z)
            };

            player_info[i].time_since_last_packet += TARGET_SPF;

            Vector3f pos = {
                -(player_info[i].current.position.x + pos_interp.x),
                -(player_info[i].current.position.y + pos_interp.y),
                -(player_info[i].current.position.z + pos_interp.z)
            };
            Vector3f rotation = {
                -(player_info[i].current.angle_v + DEG(angle_v_interp))+90.0f,
                -(player_info[i].current.angle_h + DEG(angle_h_interp))+90.0f,
                0.0f
            };

            Vector3f scale    = {1.0f, 1.0f, 1.0f};

            mesh_render(&obj, pos, rotation, scale);
        }

        // sphere

        float angle = DEG(sinf(0.1f*world.time));

#if 0
        my_sphere.pos.x = 0.0f+5.0f;my_sphere.pos.y = -15.0f;my_sphere.pos.z = 0.0f+5.0f;
        my_sphere.rotation.x = 0.0f; my_sphere.rotation.y = 0.0f;my_sphere.rotation.z = 0.0f;
        my_sphere.scale.x = 1.0f;my_sphere.scale.y = 1.0f;my_sphere.scale.z = 1.0f;

        sphere_render(&my_sphere);
#else
        my_sphere.rotation.x = 0.0f; my_sphere.rotation.y = 0.0f;my_sphere.rotation.z = 0.0f;
        my_sphere.scale.x = 1.0f;my_sphere.scale.y = 1.0f;my_sphere.scale.z = 1.0f;

        
        for(int i = 0; i < 10; ++i)
        {
            for(int j = 0; j < 10; ++j)
            {
                for(int k = 0; k < 10; ++k)
                {
                    my_sphere.pos.x = 0.0f+5.0f*i;my_sphere.pos.y = -10.0f-5*k;my_sphere.pos.z = 0.0f+5.0f*j;

                    Vector3f c = {1.0f-(i/10.0f),1.0f-(j/10.0f),1.0f-(k/10.0f)};
                
                    sphere_render(&my_sphere,c);
                }
            }

        }
#endif

        sky_render();

        Vector3f color = {1.0f,1.0f,1.0f};

        if(is_client)
            text_print(10.0f,25.0f,"Connected to Server",color);
        else
            text_print(10.0f,25.0f,"Local Game",color);

        char text_num_players[16] = {0};
        snprintf(text_num_players,16,"Player Count: %d",num_other_players+1);
        text_print(10.0f,50.0f,text_num_players,color);

        color.x = 0.60f; color.y = 0.00f; color.z = 0.60f;
        text_print(10.0f,100.0f,player.name,color);

        color.x = 0.0f; color.y = 1.0f; color.z = 1.0f;
        for(int i = 0; i < num_other_players;++i)
        {
            if(player_info[i].highlighted)
                text_print(view_width/2.0f - 75.0f,view_height - 30.0f,player_info[i].player_name,color);
        }

        // reticule
        color.x = 1.0f; color.y = 1.0f; color.z = 1.0f;
        text_print(view_width/2.0f-1,view_height/2.0f,".",color);
    }

    glfwSwapBuffers(window);
}

