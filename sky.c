#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "util.h"
#include "math3d.h"
#include "texture.h"
#include "transform.h"
#include "shader.h"
#include "sky.h"

const float sky_vertices[] = {
    -1.0f,-1.0f,-1.0f,
    +1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,+1.0f,
    +1.0f,-1.0f,+1.0f,
    -1.0f,+1.0f,-1.0f,
    +1.0f,+1.0f,-1.0f,
    -1.0f,+1.0f,+1.0f,
    +1.0f,+1.0f,+1.0f
};

const u32 sky_indices2[] = {
    0,1,4,1,5,4,
    1,3,5,3,7,5,
    3,2,7,2,6,7,
    2,0,6,0,4,6,
    0,1,2,1,3,2,
    4,5,6,5,7,6
};

const u32 sky_indices[] = {
    0,4,1,1,4,5,
    1,5,3,3,5,7,
    3,7,2,2,7,6,
    2,6,0,0,6,4,
    0,2,1,1,2,3,
    4,6,5,5,6,7
};

static GLuint sky_vao;
static GLuint sky_vbo;
static GLuint sky_ibo;

void sky_init()
{
    glGenVertexArrays(1, &sky_vao);
    glBindVertexArray(sky_vao);

    glGenBuffers(1, &sky_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sky_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sky_vertices), &sky_vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&sky_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sky_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sky_indices), sky_indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glUseProgram(sky_program);
    shader_set_int(sky_program,"skybox",0);
}

void sky_render()
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(sky_program);

    world_set_scale(100.0f,100.0f,100.0f);
    world_set_rotation(0.0f,0.0f,0.0f);
    world_set_position(0.0f,0.0f,0.0f);

    Matrix4f* wvp = get_wvp_transform();

    shader_set_mat4(sky_program, "wvp", wvp);

    glBindVertexArray(sky_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_cube);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sky_ibo);
    glDrawElements(GL_TRIANGLES, sizeof(sky_indices), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}
