#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "util.h"
#include "math3d.h"
#include "texture.h"
#include "transform.h"
#include "shader.h"
#include "camera.h"
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

const u32 sky_indices[] = {
    0,1,4,1,5,4,
    1,3,5,3,7,5,
    3,2,7,2,6,7,
    2,0,6,0,4,6,
    0,2,1,1,2,3,
    4,5,6,5,7,6
};

static GLuint sky_program;

static GLuint sky_vao;
static GLuint sky_vbo;
static GLuint sky_ibo;

void sky_init()
{
    shader_build_program(&sky_program,
        "shaders/skybox.vert.glsl",
        "shaders/skybox.frag.glsl"
    );

    glGenVertexArrays(1, &sky_vao);
    glGenBuffers(1, &sky_vbo);

    glBindVertexArray(sky_vao);
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

	world_set_scale(1.0f, 1.0f, 1.0f);
	world_set_rotation(0.0f, 0.0f, 180.0f);
	world_set_position(
            -camera.position.x-camera.player_offset.x,
            -camera.position.y-camera.player_offset.y,
            -camera.position.z-camera.player_offset.z
    );

    Matrix4f* wvp = get_wvp_transform();
    shader_set_mat4(sky_program, "wvp", wvp);

    glBindVertexArray(sky_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_cube);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sky_ibo);
    glDrawElements(GL_TRIANGLES, sizeof(sky_indices), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glUseProgram(0);
}
