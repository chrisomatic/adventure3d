#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/glew.h>

#include "util/stb_image.h"

#include "util.h"
#include "math3d.h"
#include "texture.h"
#include "terrain.h"
#include "shader.h"
#include "camera.h"
#include "transform.h"
#include "mesh.h"
#include "net.h"
#include "light.h"

#define TERRAIN_SCALE_FACTOR 2.0f
#define TERRAIN_DETAIL_LEVEL 1.0f

Mesh terrain = {0};

GLuint texture_terrain = {0};

static GLuint terrain_program;

static float* terrain_heights;
static int terrain_heights_width;

static float terrain_scale;
static float terrain_pos;

void terrain_render()
{
    glUseProgram(terrain_program);

    world_set_scale(terrain_scale,1.0f,terrain_scale);
    world_set_rotation(0.0f,0.0f,0.0f);
    world_set_position(-terrain_pos,0.0f,-terrain_pos);

    Matrix4f* world = get_world_transform();
    Matrix4f* wvp   = get_wvp_transform();

    shader_set_mat4(terrain_program,"world",world);
    shader_set_mat4(terrain_program,"wvp",wvp);

    shader_set_int(terrain_program,"wireframe",show_wireframe);

    shader_set_float(terrain_program,"dl.ambient_intensity",sunlight.base.ambient_intensity);
    shader_set_float(terrain_program,"dl.diffuse_intensity",sunlight.base.diffuse_intensity);
    shader_set_vec3(terrain_program,"dl.color",sunlight.base.color.x, sunlight.base.color.y,sunlight.base.color.z);
    shader_set_vec3(terrain_program,"dl.direction",sunlight.direction.x,sunlight.direction.y,sunlight.direction.z);

    /*
    glUniformMatrix4fv(world_location,1,GL_TRUE,(const GLfloat*)world);
    glUniformMatrix4fv(wvp_location,1,GL_TRUE,(const GLfloat*)wvp);

    glUniform1i(wireframe_location, show_wireframe);

    glUniform3f(dir_light_location.color, sunlight.base.color.x, sunlight.base.color.y, sunlight.base.color.z);
    glUniform1f(dir_light_location.ambient_intensity, sunlight.base.ambient_intensity);
    glUniform3f(dir_light_location.direction, sunlight.direction.x, sunlight.direction.y, sunlight.direction.z);
    glUniform1f(dir_light_location.diffuse_intensity, sunlight.base.diffuse_intensity);
    */

    texture_bind(&texture_terrain,GL_TEXTURE0);

    glBindVertexArray(terrain.vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,terrain.ibo);

    if(show_wireframe)
        glDrawElements(GL_LINES, terrain.num_indices, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_TRIANGLES, terrain.num_indices, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    texture_unbind();
    glUseProgram(0);
}

static bool get_terrain_points_and_pos(float x, float z, Vector3f* p1, Vector3f* p2, Vector3f* p3, Vector2f* pos)
{
    float terrain_x = -x + terrain_pos;
    float terrain_z = -z + terrain_pos;

    float grid_square_size = terrain_scale * (1.0f / terrain_heights_width);

    int grid_x = (int)floor(terrain_x / grid_square_size);
    if(grid_x < 0 || grid_x >= terrain_heights_width)
        return false;

    int grid_z = (int)floor(terrain_z / grid_square_size);
    if(grid_z < 0 || grid_z >= terrain_heights_width)
        return false;

    float x_coord = fmod(terrain_x,grid_square_size)/grid_square_size;
    float z_coord = fmod(terrain_z,grid_square_size)/grid_square_size;
    
    //printf("grid x: %d z: %d\n",grid_x, grid_z);

    int g = terrain_heights_width+1;

    if (x_coord <= (1.0f-z_coord))
    {
        p1->x = 0; p1->y = terrain_heights[g*grid_x+grid_z];     p1->z = 0;
        p2->x = 1; p2->y = terrain_heights[(g*grid_x+1)+grid_z]; p2->z = 1;
        p3->x = 0; p3->y = terrain_heights[g*grid_x+(grid_z+1)]; p3->z = 1;
    }
    else
    {
        p1->x = 1; p1->y = terrain_heights[(g*grid_x+1)+grid_z]; p1->z = 0;
        p2->x = 1; p2->y = terrain_heights[(g*grid_x+1)+grid_z]; p2->z = 1;
        p3->x = 0; p3->y = terrain_heights[g*grid_x+(grid_z+1)]; p3->z = 1;
    }

    if(pos == NULL)
        return true;

    pos->x = x_coord;
    pos->y = z_coord;

    return true;
}

void terrain_get_stats(float x, float z, float* height, Vector3f* ret_norm)
{
    Vector3f a  = {0};
    Vector3f b  = {0};
    Vector3f c  = {0};
    Vector2f pos2 = {0};

    bool res = get_terrain_points_and_pos(x,z,&a,&b,&c,&pos2);

    if(res)
    {
        *height = barry_centric(a,b,c,pos2);
        get_normal_v3f(a,b,c,ret_norm);
    }
    else
    {
        *height = 0.0f;
        memset(ret_norm,0,sizeof(Vector3f));
    }
}

void terrain_build(const char* heightmap)
{

    shader_build_program(&terrain_program,
        "shaders/terrain.vert.glsl",
        "shaders/terrain.frag.glsl"
    );

    texture_load2d(&texture_terrain,"textures/grass.png");

    glGenVertexArrays(1, &terrain.vao);
    glBindVertexArray(terrain.vao);

    int x,y,n;
    unsigned char* heightdata = stbi_load(heightmap, &x, &y, &n, 1);

    if(!heightdata)
    {
        printf("Failed to load file (%s)",heightmap);
        return;
    }
    
    printf("Loaded file %s. w: %d h: %d channels: %d\n",heightmap,x,y,n);

    terrain_heights_width = (x-1)*TERRAIN_DETAIL_LEVEL;
    int terrain_heights_width_p1 = (x)*TERRAIN_DETAIL_LEVEL;

    terrain_scale = TERRAIN_SCALE_FACTOR*terrain_heights_width;
    terrain_pos = terrain_scale / 2.0f;

    int terrain_vertex_count = terrain_heights_width_p1*terrain_heights_width_p1;
    int terrain_index_count = terrain_heights_width*terrain_heights_width*6;

    Vertex* terrain_vertices = calloc(terrain_vertex_count,sizeof(Vertex));
    u32*    terrain_indices  = calloc(terrain_index_count,sizeof(u32));

    terrain_heights = calloc(terrain_heights_width_p1*terrain_heights_width_p1,sizeof(float));

    if(!terrain_heights)
    {
        printf("Failed to allocate memory for terrain height array");
        return;
    }

    const float interval = 1.0f/terrain_heights_width;

    //printf("===== TERRAIN =====\n");
    
    for(int i = 0; i < terrain_heights_width_p1; ++i)
    {
        for(int j = 0; j < terrain_heights_width_p1; ++j)
        {
            float xlookup = (i*terrain_heights_width_p1) / TERRAIN_DETAIL_LEVEL;
            float ylookup = j / TERRAIN_DETAIL_LEVEL;

            int xindex = (int)xlookup;
            int yindex = (int)ylookup;

            float mod_x = (xlookup - xindex);
            float mod_y = (ylookup - yindex);

            int index = xindex + yindex;
            int width = terrain_heights_width_p1 / TERRAIN_DETAIL_LEVEL;

            float h00 = heightdata[index];
            float h01 = heightdata[index+width];
            float h10 = heightdata[index+1];
            float h11 = heightdata[index+width+1];

            float height_x = (1.0f-mod_x)*h00 + (mod_x)*h01;
            float height_y = (1.0f-mod_y)*h10 + (mod_y)*h11;

            float norm_height = (height_x + height_y) / 2.0f;
            norm_height /= 8.0f;

            terrain_heights[index] = norm_height;

            terrain_vertices[index].position.x = i*interval;
            terrain_vertices[index].position.y = -terrain_heights[index];
            terrain_vertices[index].position.z = j*interval;

            terrain_vertices[index].tex_coord.x = 10*i*interval;
            terrain_vertices[index].tex_coord.y = 10*j*interval;
        }
    }

    int index = 0;

    for(int i = 0; i < terrain_index_count; i += 6)
    {
        if((i/6) > 0 && (i/6) % terrain_heights_width == 0)
            index += 6;

        terrain_indices[i]   = (index / 6);
        terrain_indices[i+1] = terrain_indices[i] + terrain_heights_width_p1;
        terrain_indices[i+2] = terrain_indices[i] + 1;
        terrain_indices[i+3] = terrain_indices[i+1];
        terrain_indices[i+4] = terrain_indices[i+1] + 1;
        terrain_indices[i+5] = terrain_indices[i+2];

        index+=6;
    }


    terrain.num_vertices = terrain_vertex_count;
    terrain.vertices = malloc(terrain.num_vertices*sizeof(Vertex));
    memcpy(terrain.vertices,terrain_vertices,terrain.num_vertices*sizeof(Vertex));

    terrain.num_indices = terrain_index_count;
    terrain.indices = malloc(terrain.num_indices*sizeof(u32));
    memcpy(terrain.indices,terrain_indices,terrain.num_indices*sizeof(u32));

    calc_vertex_normals(terrain.indices, terrain.num_indices, terrain.vertices, terrain.num_vertices);

 	glGenBuffers(1, &terrain.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
	glBufferData(GL_ARRAY_BUFFER, terrain.num_vertices*sizeof(Vertex), terrain.vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&terrain.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain.num_indices*sizeof(u32), terrain.indices, GL_STATIC_DRAW);

    free(terrain_vertices);
    free(terrain_indices);
}
