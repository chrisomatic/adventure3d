#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <GL/glew.h>

#include "util.h"
#include "math3d.h"
#include "shader.h"
#include "texture.h"
#include "transform.h"
#include "mesh.h"
#include "light.h"

#include "sphere.h"

#define MAX_SUBDIVISIONS 32

// platonic solid
const Vertex base_vertices[] = 
{
    {{-0.5f,+0.0f,-0.5f},{0.0f,0.0f},{0.0f,0.0f,0.0f}},
    {{+0.5f,+0.0f,-0.5f},{0.0f,0.5f},{0.0f,0.0f,0.0f}},
    {{+0.5f,+0.0f,+0.5f},{0.5f,0.5f},{0.0f,0.0f,0.0f}},
    {{-0.5f,+0.0f,+0.5f},{0.0f,0.5f},{0.0f,0.0f,0.0f}},
    {{+0.0f,+1.0f,+0.0f},{1.0f,0.0f},{0.0f,0.0f,0.0f}},
    {{+0.0f,-1.0f,+0.0f},{1.0f,1.0f},{0.0f,0.0f,0.0f}}
};

const u32 base_indices[] =
{
    4,1,0,
    4,1,0,
    4,2,1,
    4,2,1,
    4,3,2,
    4,3,2,
    4,0,3,
    4,0,3,
    5,0,1,
    5,1,2,
    5,2,3,
    5,3,0
};

static void subdivide_sphere(Sphere* s)
{
    for(int i = 0; i < s->num_indices; i+=3)
    {
        u32 i1 = s->indices[i];
        u32 i3 = s->indices[i+1];
        u32 i4 = s->indices[i+2];
    }
}

bool sphere_create(u32 num_subdivisions,float radius, Sphere* s)
{
    if(num_subdivisions > MAX_SUBDIVISIONS)
    {
        printf("Num subdivisions is too large (%d), must be less than %d", num_subdivisions, MAX_SUBDIVISIONS);
        return false;
    }

    if(!s) s = calloc(1, sizeof(Sphere));

    s->pos.x      = 0.0f; s->pos.y      = 0.0f; s->pos.z      = 0.0f;
    s->rotation.x = 0.0f; s->rotation.y = 0.0f; s->rotation.z = 0.0f;
    s->scale.x    = 1.0f; s->scale.y    = 1.0f; s->scale.z    = 1.0f;

    glGenVertexArrays(1, &s->vao);
    glBindVertexArray(s->vao);

    s->num_vertices = sizeof(base_vertices)/sizeof(Vertex); // initial value.
    s->vertices = malloc(s->num_vertices*sizeof(Vertex)); // 8 base tris * 4 per subdivision * 3 per tri 
    memcpy(s->vertices,base_vertices,sizeof(base_vertices));

    s->num_indices = sizeof(base_indices)/sizeof(u32); // initial value.
    s->indices = malloc(s->num_indices*sizeof(u32)); // 8 base tris * 4 per subdivision * 3 per tri 
    memcpy(s->indices,base_indices,sizeof(base_indices));

    for(int i = 0; i < s->num_indices; i += 3)
    {
        printf("%d: %u %u %u\n",i, s->indices[i], s->indices[i+1], s->indices[i+2]);
    }

    /*
    for(int i = 0; i < num_subdivisions; ++i)
        subdivide_sphere(s);
    */

    calc_vertex_normals(s->indices, s->num_indices, s->vertices, s->num_vertices);

 	glGenBuffers(1, &s->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
	glBufferData(GL_ARRAY_BUFFER, s->num_vertices*sizeof(Vertex), s->vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &s->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, s->num_indices*sizeof(u32), s->indices, GL_STATIC_DRAW);

    return true;
}

void sphere_render(Sphere* s)
{
    glUseProgram(program);
    
    world_set_position(s->pos.x,s->pos.y,s->pos.z);
    world_set_rotation(s->rotation.x,s->rotation.y,s->rotation.z);
    world_set_scale(s->scale.x,s->scale.y,s->scale.z);

    Matrix4f* world = get_world_transform();
    Matrix4f* wvp = get_wvp_transform();

    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)wvp);

    glUniform1i(wireframe_location, show_wireframe);

    glUniform3f(dir_light_location.color, sunlight.base.color.x, sunlight.base.color.y, sunlight.base.color.z);
    glUniform1f(dir_light_location.ambient_intensity, sunlight.base.ambient_intensity);
    glUniform3f(dir_light_location.direction, sunlight.direction.x, sunlight.direction.y, sunlight.direction.z);
    glUniform1f(dir_light_location.diffuse_intensity, sunlight.base.diffuse_intensity);

    glBindVertexArray(s->vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,s->ibo);
    if(show_wireframe)
        glDrawElements(GL_LINES, s->num_indices, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_TRIANGLES, s->num_indices, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    glUseProgram(0);

}
