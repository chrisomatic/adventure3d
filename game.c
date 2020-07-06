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
#include "import.h"

// =========================
// Global Vars
// =========================

GLuint vbo,ibo;

Vertex vertices[] =
{
    {{-1.0f,-1.0f, 0.5773f}  ,{0.0f,0.0f}, {0.0f,0.0f,0.0f}},
    {{ 0.0f,-1.0f, -1.15475f},{0.5f,0.0f}, {0.0f,0.0f,0.0f}},
    {{ 1.0f,-1.0f, 0.5773f}  ,{1.0f,0.0f}, {0.0f,0.0f,0.0f}},
    {{ 0.0f, 1.0f, 0.0f}     ,{0.5f,1.0f}, {0.0f,0.0f,0.0f}},
};

unsigned int indices[] = 
{
    0,3,1,
    1,3,2,
    2,3,0,
    0,1,2
};

Texture texture = {0};

typedef struct
{
    GLuint vbo;
    GLuint ibo;
    Vector3f pos;
} Object;

#define MAX_OBJECTS 100
Object objects[MAX_OBJECTS] = {0};
int num_objects = 0;

// =========================
// Function Prototypes
// =========================

void init();
void deinit();
void simulate();
void render();

void create_vbo();
void load_textures();
void init_objects();

// =========================
// Main Loop
// =========================

int main()
{

    import_stlb("models/test.stl");

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

    world_set_scale(1.0f,1.0f,1.0f);
    world_set_rotation(10*world.time,10*world.time,0.0f);
    world_set_position(0.0f,10.0f*sinf(world.time),40.0f); //ABS(50.0f*sinf(world.time)));

    Matrix4f* _world = get_world_transform();
    Matrix4f* _wvp   = get_wvp_transform();

    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)_world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)_wvp);

    glUniform3f(dir_light_location.color, light.color.x, light.color.y, light.color.z);
    glUniform1f(dir_light_location.ambient_intensity, light.ambient_intensity);

    Vector3f dir;
    copy_v3f(&dir, &light.direction);
    normalize_v3f(&dir);

    glUniform3f(dir_light_location.direction, dir.x, dir.y, dir.z);
    glUniform1f(dir_light_location.diffuse_intensity, light.diffuse_intensity);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for(int i = 0; i < num_objects; ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, objects[i].vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,objects[i].ibo);
        texture_bind(&texture,GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

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

    // VAO
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    printf("Creating vertex objects.\n");
    init_objects();

    printf("Loading shaders.\n");
    shader_load_all();

    printf("Loading textures.\n");
    load_textures();

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

void init_objects()
{
    num_objects = 1;

    calc_vertex_normals(indices, COUNT_OF(indices), vertices, COUNT_OF(vertices));

 	glGenBuffers(1, &objects[0].vbo);
	glBindBuffer(GL_ARRAY_BUFFER, objects[0].vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&objects[0].ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[0].ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
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

