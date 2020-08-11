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
#include "light.h"

#include "sphere.h"

#define SMOOTHEST_SUBDIVISIONS 32

// platonic solid
const Vector3f base_vertices[] = 
{
    {-1.0f,+0.0f,-1.0f},
    {+1.0f,+0.0f,-1.0f},
    {+1.0f,+0.0f,+1.0f},
    {-1.0f,+0.0f,+1.0f},
    {+0.5f,+1.0f,+0.5f},
    {+0.5f,-1.0f,+0.5f}
};

const u8 base_indices[] =
{
    0,1,4,
    1,2,4,
    2,3,4,
    3,0,4,
    0,1,5,
    1,2,5,
    2,3,5,
    3,0,5
};

void sphere_render(float smooth_factor, Vector3f pos, Vector3f rotation, Vector3f scale)
{
    // smooth_factor 0.0 -> 1.0
    glUseProgram(program);
    
    // render current player
    world_set_scale(scale.x,scale.y,scale.z);
    world_set_rotation(rotation.x,rotation.y,rotation.z);
    world_set_position(pos.x,pos.y,pos.z);

    Matrix4f* world = get_world_transform();
    Matrix4f* wvp = get_wvp_transform();

    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)wvp);

    glUniform1i(sampler, 0);
    glUniform1i(wireframe_location, show_wireframe);

    glUniform3f(dir_light_location.color, sunlight.base.color.x, sunlight.base.color.y, sunlight.base.color.z);
    glUniform1f(dir_light_location.ambient_intensity, sunlight.base.ambient_intensity);
    glUniform3f(dir_light_location.direction, sunlight.direction.x, sunlight.direction.y, sunlight.direction.z);
    glUniform1f(dir_light_location.diffuse_intensity, sunlight.base.diffuse_intensity);

    glBindVertexArray(mesh->vao);


}
