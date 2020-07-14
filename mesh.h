#pragma once

#include <stdbool.h>
#include "util.h"

typedef struct
{
    char* name;
    GLuint texture;
} Material;

typedef struct
{
    Material mat;

    GLuint vao;
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

void mesh_init_all();
void mesh_load_model(ModelFormat format, const char* file_path, Mesh* mesh);
void mesh_render(Mesh* mesh);
