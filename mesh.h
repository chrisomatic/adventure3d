#pragma once

#include "util.h"
#include "texture.h"

typedef struct
{
    char* name;
    Texture texture;
} Material;

typedef struct
{
    Material mat;

    GLuint vbo;
    GLuint ibo;

    u32 num_vertices;
    Vertex* vertices; 

    u32 num_indices;
    u32* indices;

    u32 material_index;

} Mesh;

typedef enum
{
    MODEL_FORMAT_STL,
    MODEL_FORMAT_OBJ,
} ModelFormat;

extern bool show_wireframe;

extern Mesh obj;
extern Mesh terrain;

void mesh_init_all();
void mesh_load_model(ModelFormat format, const char* file_path, Mesh* mesh);
void mesh_render(Mesh* mesh);
