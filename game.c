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

// =========================
// Global Vars
// =========================

GLuint vbo,ibo;

Texture texture = {0};
Mesh obj = {0};
Mesh ground = {0};

// =========================
// Function Prototypes
// =========================

void init();
void deinit();
void simulate();
void render();

void create_vbo();
void load_textures();
void init_meshes();

// =========================
// Main Loop
// =========================

int main()
{
    init();

    double lasttime = glfwGetTime();

    for(;;)
    {
        glfwPollEvents();
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(window) != 0)
            break;

        simulate();
        render();

        // wait for next frame
        while(glfwGetTime() < lasttime + TARGET_SPF)
            usleep(100);

        lasttime += TARGET_SPF;
    }

    deinit();

    return 0;
}

// =========================
// Functions
// =========================

void simulate()
{
    world.time += TARGET_SPF;

    //printf("\ntime: %f\n",world.time);

    camera_update();

    Vector3f dir;
    copy_v3f(&dir, &light.direction);
    normalize_v3f(&dir);

    glUniform3f(dir_light_location.direction, dir.x, dir.y, dir.z);
    glUniform1f(dir_light_location.diffuse_intensity, light.diffuse_intensity);
}

void render()
{
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4f* _world;
    Matrix4f* _wvp;

    world_set_scale(100.0f,1.0f,100.0f);
    world_set_rotation(0.0f,0.0f,0.0f);
    world_set_position(0.0f,0.0f,0.0f);

    _world = get_world_transform();
    _wvp   = get_wvp_transform();

    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

    glUniform3f(dir_light_location.color, light.color.x, light.color.y, light.color.z);
    glUniform1f(dir_light_location.ambient_intensity, light.ambient_intensity);

    mesh_render(&ground);

    world_set_scale(1.0f,1.0f,1.0f);
    world_set_rotation(10*world.time,10*world.time,0.0f);
    world_set_position(0.0f,10.0f*sinf(world.time),40.0f); //ABS(50.0f*sinf(world.time)));

    _world = get_world_transform();
    _wvp   = get_wvp_transform();

    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

    glUniform3f(dir_light_location.color, light.color.x, light.color.y, light.color.z);
    glUniform1f(dir_light_location.ambient_intensity, light.ambient_intensity);

    mesh_render(&obj);

    // Swap buffers
    glfwSwapBuffers(window);
}

void init_lighting()
{
    light.color.x = 1.0f;
    light.color.y = 1.0f;
    light.color.z = 1.0f;

    light.ambient_intensity = 0.50f;
    light.diffuse_intensity = 0.75f;

    light.direction.x = 1.00f;
    light.direction.y = 0.00f;
    light.direction.z = 0.00f;
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

    printf("GL version: %s\n",glGetString(GL_VERSION));
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    // VAO
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    printf("Loading textures.\n");
    load_textures();

    printf("Loading shaders.\n");
    shader_load_all();

    printf("Creating meshes.\n");
    init_meshes();

    camera_init();
    init_world();
    init_lighting();

    glUniform1i(sampler, 0);

    // put pointer in center of window
    glfwSetCursorPos(window, camera.cursor.x, camera.cursor.y);
}

void deinit()
{
    glDeleteBuffers(1, &vbo);

    shader_deinit();
    window_deinit();
}

void init_meshes()
{
    mesh_load_model(MODEL_FORMAT_STL,"models/donut.stl",&obj);
    calc_vertex_normals(obj.indices, obj.num_indices, obj.vertices, obj.num_vertices);

 	glGenBuffers(1, &obj.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, obj.vbo);
	glBufferData(GL_ARRAY_BUFFER, obj.num_vertices*sizeof(Vertex), obj.vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&obj.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.num_indices*sizeof(u32), obj.indices, GL_STATIC_DRAW);

    // ground
    Vertex floor_vertices[] = {
        {{0.0f,0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f,0.0f}},
        {{0.0f,0.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f,0.0f}},
        {{1.0f,0.0f,0.0f},{1.0f,0.0f},{0.0f,0.0f,0.0f}},
        {{1.0f,0.0f,1.0f},{1.0f,1.0f},{0.0f,0.0f,0.0f}}
    };

    u32 floor_indices[] = {
        0,2,1,
        2,3,1
    };

    ground.num_vertices = 4;
    ground.vertices = malloc(ground.num_vertices*sizeof(Vertex));
    memcpy(ground.vertices,floor_vertices,ground.num_vertices*sizeof(Vertex));

    ground.num_indices = 6;
    ground.indices = malloc(ground.num_indices*sizeof(u32));
    memcpy(ground.indices,floor_indices,ground.num_indices*sizeof(u32));

    calc_vertex_normals(ground.indices, ground.num_indices, ground.vertices, ground.num_vertices);

 	glGenBuffers(1, &ground.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ground.vbo);
	glBufferData(GL_ARRAY_BUFFER, ground.num_vertices*sizeof(Vertex), ground.vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&ground.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ground.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ground.num_indices*sizeof(u32), ground.indices, GL_STATIC_DRAW);

    memcpy(&obj.mat.texture,&texture,sizeof(Texture));
    memcpy(&ground.mat.texture,&texture,sizeof(Texture));

}

void load_textures()
{
    bool success;

    success = texture_load(&texture,"textures/test.png");
    if(!success)
    {
        printf("Failed to load texture!\n");
        return;
    }
}

