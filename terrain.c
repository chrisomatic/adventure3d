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
#include "mesh.h"

#define TERRAIN_GRANULARITY    256
#define TERRAIN_GRANULARITY_P1 (TERRAIN_GRANULARITY+1)
#define TERRAIN_VERTEX_COUNT   (TERRAIN_GRANULARITY_P1*TERRAIN_GRANULARITY_P1)
#define TERRAIN_INDEX_COUNT    (TERRAIN_GRANULARITY*TERRAIN_GRANULARITY*6)

Mesh terrain = {0};

void terrain_render()
{
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

static unsigned char* heightdata;

float terrain_get_height(float x, float z)
{
    if(!heightdata)
        return 0.0f;

    int _x = floor(-x);
    if(_x < 0 || _x > TERRAIN_GRANULARITY_P1)
        return 0.0f;

    int _z = floor(-z);
    if(_z < 0 || _z > TERRAIN_GRANULARITY_P1)
        return 0.0f;

    return (10*heightdata[_z*TERRAIN_GRANULARITY_P1+_x]/255.0f);
}

void terrain_build(const char* heightmap)
{
    int x,y,n;
    heightdata = stbi_load(heightmap, &x, &y, &n, 0);

    if(!heightdata)
    {
        printf("Failed to load file (%s)",heightmap);
        return;
    }
    
    printf("Loaded file %s. w: %d h: %d channels: %d\n",heightmap,x,y,n);

    /*
    for(int i = 0; i < x; ++i){
        for(int j = 0; j < y; ++j){
            printf("%u ",heightdata[i*x+j]);
        }
        printf("\n");
    }
    */

    Vertex terrain_vertices[TERRAIN_VERTEX_COUNT] = {0};
    u32    terrain_indices[TERRAIN_INDEX_COUNT]   = {0};

    const float interval = 1.0f/TERRAIN_GRANULARITY;

    //printf("===== TERRAIN =====\n");
    
    for(int i = 0; i < x; ++i)
    {
        for(int j = 0; j < y; ++j)
        {
            int index = i*(x)+j;

            terrain_vertices[index].position.x = i*interval;
            terrain_vertices[index].position.y = 10*(heightdata[index]/255.0f);
            terrain_vertices[index].position.z = j*interval;

            terrain_vertices[index].tex_coord.x = 10*i*interval;
            terrain_vertices[index].tex_coord.y = 10*j*interval;

            /*
            printf("%d: %f %f %f\n",
                index,
                terrain_vertices[index].position.x,
                terrain_vertices[index].position.y,
                terrain_vertices[index].position.z);
            */
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
