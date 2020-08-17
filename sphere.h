#pragma once

typedef struct
{
    Vector3f pos;
    Vector3f rotation;
    Vector3f scale;

    GLuint vao;
    GLuint vbo;
    GLuint ibo;

    u32 num_vertices;
    Vertex* vertices; 

    u32 num_indices;
    u32* indices;
} Sphere;


bool sphere_create(u32 num_subdivisions,float radius, Sphere* s);
void sphere_render(Sphere* s);
