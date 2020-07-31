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

#define TERRAIN_GRANULARITY    256
#define TERRAIN_GRANULARITY_P1 (TERRAIN_GRANULARITY+1)
#define TERRAIN_VERTEX_COUNT   (TERRAIN_GRANULARITY_P1*TERRAIN_GRANULARITY_P1)
#define TERRAIN_INDEX_COUNT    (TERRAIN_GRANULARITY*TERRAIN_GRANULARITY*6)

Mesh terrain = {0};

static float terrain_heights[TERRAIN_GRANULARITY_P1][TERRAIN_GRANULARITY_P1] = {0.0f};

const float terrain_scale = 256.0f;
const float terrain_pos = terrain_scale / 2.0f;

void terrain_render()
{
    glUseProgram(program);

    world_set_scale(terrain_scale,1.0f,terrain_scale);
    world_set_rotation(0.0f,0.0f,0.0f);
    world_set_position(-terrain_pos,0.0f,-terrain_pos);

    Matrix4f* world = get_world_transform();
    Matrix4f* wvp   = get_wvp_transform();

    shader_set_mat4(program, "world", world);
    shader_set_mat4(program, "wvp", wvp);

    glUniform1i(wireframe_location, show_wireframe);
    glUniform3f(camera_position_location, camera.position.x,camera.position.y,camera.position.z);

    glBindVertexArray(terrain.vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,terrain.ibo);

    if(terrain.mat.texture)
        texture_bind(&terrain.mat.texture,GL_TEXTURE0);

    if(show_wireframe)
        glDrawElements(GL_LINES, terrain.num_indices, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_TRIANGLES, terrain.num_indices, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    texture_unbind();
}

static void get_terrain_points_and_pos(float x, float z, Vector3f* p1, Vector3f* p2, Vector3f* p3, Vector2f* pos)
{
    float terrain_x = -x + terrain_pos;
    float terrain_z = -z + terrain_pos;

    float grid_square_size = terrain_scale * (1.0f / TERRAIN_GRANULARITY);

    int grid_x = (int)floor(terrain_x / grid_square_size);
    if(grid_x < 0 || grid_x >= TERRAIN_GRANULARITY)
        return;

    int grid_z = (int)floor(terrain_z / grid_square_size);
    if(grid_z < 0 || grid_z >= TERRAIN_GRANULARITY)
        return;

    float x_coord = fmod(terrain_x,grid_square_size)/grid_square_size;
    float z_coord = fmod(terrain_z,grid_square_size)/grid_square_size;
    
    //printf("grid x: %d z: %d\n",grid_x, grid_z);

    if (x_coord <= (1.0f-z_coord))
    {
        p1->x = 0; p1->y = terrain_heights[grid_x][grid_z];   p1->z = 0;
        p2->x = 1; p2->y = terrain_heights[grid_x+1][grid_z]; p2->z = 1;
        p3->x = 0; p3->y = terrain_heights[grid_x][grid_z+1]; p3->z = 1;
    }
    else
    {
        p1->x = 1; p1->y = terrain_heights[grid_x+1][grid_z]; p1->z = 0;
        p2->x = 1; p2->y = terrain_heights[grid_x+1][grid_z]; p2->z = 1;
        p3->x = 0; p3->y = terrain_heights[grid_x][grid_z+1]; p3->z = 1;
    }

    if(pos == NULL)
        return;

    pos->x = x_coord;
    pos->y = z_coord;
}

void terrain_get_stats(float x, float z, float* height, float* angle_xy, float* angle_zy)
{
    Vector3f a  = {0};
    Vector3f b  = {0};
    Vector3f c  = {0};
    Vector2f pos2 = {0};

    get_terrain_points_and_pos(x,z,&a,&b,&c,&pos2);

    *height = barry_centric(a,b,c,pos2);

    if(angle_xy == NULL && angle_zy == NULL)
        return;

    float adj = terrain_scale * (1.0f / TERRAIN_GRANULARITY);

    float xy = a.y - b.y;
    float zy = c.y - b.y;

    *angle_xy = DEG(atan(xy/adj));
    *angle_zy = DEG(atan(zy/adj));

    /*
    float b_dot_a = dot_product_v3f(&b,&a);
    float b_dot_c = dot_product_v3f(&b,&c);

    float mag_a = magnitude_v3f(&a);
    float mag_c = magnitude_v3f(&c);
    float mag_b = magnitude_v3f(&b);

    float dem_xy = mag_a*mag_b;
    float dem_zy = mag_c*mag_b;

    if(angle_xy != NULL)
    {
        // (a dot b) / (|a||b|)
        if(dem_xy == 0.0f)
            *angle_xy = 0.0f;
        else 
            *angle_xy = acos(b_dot_a / dem_xy);
        
        *angle_xy = DEG(*angle_xy);
    }

    if(angle_zy != NULL)
    {
        // (c dot b) / (|c||b|)
        if(dem_zy == 0.0f)
            *angle_zy = 0.0f;
        else
            *angle_zy = acos(b_dot_c / (dem_zy));

        *angle_zy = DEG(*angle_zy);
    }
    */
}

void terrain_build(const char* heightmap)
{
    glGenVertexArrays(1, &terrain.vao);
    glBindVertexArray(terrain.vao);

    int x,y,n;
    unsigned char* heightdata = stbi_load(heightmap, &x, &y, &n, 0);

    if(!heightdata)
    {
        printf("Failed to load file (%s)",heightmap);
        return;
    }
    
    printf("Loaded file %s. w: %d h: %d channels: %d\n",heightmap,x,y,n);

    Vertex terrain_vertices[TERRAIN_VERTEX_COUNT] = {0};
    u32    terrain_indices[TERRAIN_INDEX_COUNT]   = {0};

    const float interval = 1.0f/TERRAIN_GRANULARITY;

    //printf("===== TERRAIN =====\n");
    
    for(int i = 0; i < x; ++i)
    {
        for(int j = 0; j < y; ++j)
        {
            int index = i*(x)+j;

            float norm_height = (heightdata[index]/8.0f);
            terrain_heights[i][j] = norm_height;

            terrain_vertices[index].position.x = i*interval;
            terrain_vertices[index].position.y = -terrain_heights[i][j];
            terrain_vertices[index].position.z = j*interval;

            terrain_vertices[index].tex_coord.x = 10*i*interval;
            terrain_vertices[index].tex_coord.y = 10*j*interval;
        }
    }

    int index = 0;

    for(int i = 0; i < TERRAIN_INDEX_COUNT; i += 6)
    {
        if((i/6) > 0 && (i/6) % TERRAIN_GRANULARITY == 0)
            index += 6;

        terrain_indices[i]   = (index / 6);
        terrain_indices[i+1] = terrain_indices[i] + TERRAIN_GRANULARITY_P1;
        terrain_indices[i+2] = terrain_indices[i] + 1;
        terrain_indices[i+3] = terrain_indices[i+1];
        terrain_indices[i+4] = terrain_indices[i+1] + 1;
        terrain_indices[i+5] = terrain_indices[i+2];

        index+=6;
    }


    terrain.num_vertices = TERRAIN_VERTEX_COUNT;
    terrain.vertices = malloc(terrain.num_vertices*sizeof(Vertex));
    memcpy(terrain.vertices,terrain_vertices,terrain.num_vertices*sizeof(Vertex));

    terrain.num_indices = TERRAIN_INDEX_COUNT;
    terrain.indices = malloc(terrain.num_indices*sizeof(u32));
    memcpy(terrain.indices,terrain_indices,terrain.num_indices*sizeof(u32));

    calc_vertex_normals(terrain.indices, terrain.num_indices, terrain.vertices, terrain.num_vertices);

 	glGenBuffers(1, &terrain.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
	glBufferData(GL_ARRAY_BUFFER, terrain.num_vertices*sizeof(Vertex), terrain.vertices, GL_STATIC_DRAW);

    glGenBuffers(1,&terrain.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain.num_indices*sizeof(u32), terrain.indices, GL_STATIC_DRAW);

    memcpy(&terrain.mat.texture,&texture1,sizeof(GLuint));
}
