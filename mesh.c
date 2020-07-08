#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <GL/glew.h>

#include "util.h"
#include "math3d.h"
#include "texture.h"

#include "mesh.h"

bool show_wireframe = false;

typedef struct
{
    Vector3f normal;
    Vector3f vertex1;
    Vector3f vertex2;
    Vector3f vertex3;
    u16      attr_count;
} STL_Triangle;

typedef struct
{
    u8 header[80];
    u32 num_triangles;
    STL_Triangle* triangles;
} STL;

static bool import_stlb(const char* file_path, STL* stl)
{
    FILE* fp = fopen(file_path,"rb");

    if(!fp)
    {
        fprintf(stderr,"Failed to open file %s\n",file_path);
        return false;
    }

    memset(stl, 0, sizeof(STL));

    // read header
    fread(stl->header, 80, sizeof(u8),fp);

    // read num triangles
    fread(&stl->num_triangles,1, sizeof(u32),fp);

    // allocate space to triangles
    stl->triangles = malloc(stl->num_triangles*sizeof(STL_Triangle));

    if(!stl->triangles)
    {
        fprintf(stderr,"Failed to allocate memory for STL triangles.\n");
        return false;
    }

    // read triangles
    for(int i = 0; i < stl->num_triangles; ++i)
    {
        fread(&stl->triangles[i].normal,    3, sizeof(float),fp);
        fread(&stl->triangles[i].vertex1,   3, sizeof(float),fp);
        fread(&stl->triangles[i].vertex2,   3, sizeof(float),fp);
        fread(&stl->triangles[i].vertex3,   3, sizeof(float),fp);
        fread(&stl->triangles[i].attr_count,1, sizeof(u16)  ,fp);
    }

    fclose(fp);

    return true;
}

static bool is_vec3f_unique(Vector3f* unique_list, u32 unique_list_count, Vector3f* test_vertex, u32* matched_index)
{
    for(int i = 0; i < unique_list_count; ++i)
    {
        if(test_vertex->x == unique_list[i].x &&
           test_vertex->y == unique_list[i].y &&
           test_vertex->z == unique_list[i].z)
        {
            *matched_index = i;
            return false;
        }
    }
    return true;
}

static void print_mesh(Mesh* mesh)
{
    // print_mesh()
    printf("===== Mesh =====\n");

    printf("Num Vertices: %u\n",mesh->num_vertices);
    for(int i = 0; i < mesh->num_vertices; ++i)
    {
        printf("Vertex%04u: P %9.6f %9.6f %9.6f N %9.6f %9.6f %9.6f\n",i,mesh->vertices[i].position.x,mesh->vertices[i].position.y,mesh->vertices[i].position.z, mesh->vertices[i].normal.x,mesh->vertices[i].normal.y,mesh->vertices[i].normal.z);
    }

    printf("\n");
    printf("Num Indices: %u\n",mesh->num_indices);

    for(int i = 0; i < mesh->num_indices; ++i)
    {
        printf("%04u",mesh->indices[i]);
        
        if((i+1) % 3 == 0)
            printf(", ");
        else
            printf(" ");

        if((i+1) % 18 == 0)
            printf("\n");
    }
    printf("\n");
}


static void print_stl(STL* stl)
{
    printf("\n");
    printf("===== STL Model =====\n");
    printf("Header: %s\n",stl->header);
    printf("Num Triangles: %d\n",stl->num_triangles);

    for(int i = 0; i < stl->num_triangles; ++i)
    {
        printf("Triangle%d:\n\tn:  %f %f %f\n\tv1: %f %f %f\n\tv2: %f %f %f\n\tv3: %f %f %f\n\tattr count: %u\n",
                i,
                stl->triangles[i].normal.x,
                stl->triangles[i].normal.y,
                stl->triangles[i].normal.z,
                stl->triangles[i].vertex1.x,
                stl->triangles[i].vertex1.y,
                stl->triangles[i].vertex1.z,
                stl->triangles[i].vertex2.x,
                stl->triangles[i].vertex2.y,
                stl->triangles[i].vertex2.z,
                stl->triangles[i].vertex3.x,
                stl->triangles[i].vertex3.y,
                stl->triangles[i].vertex3.z,
                stl->triangles[i].attr_count
        );
    }
    printf("\n");
}

void mesh_load_model(ModelFormat format, const char* file_path, Mesh* mesh)
{
    u32 vertex_count = 0;
    Vector3f unique_vertices[2048] = {0};
    u32 indices[2048] = {0};
    u32 index_count = 0;

    u32 matched_index = 0;

    switch(format)
    {
        case MODEL_FORMAT_STL:
        {
            STL stl = {0};
            if(!import_stlb(file_path,&stl))
            {
                fprintf(stderr,"Failed to import mesh %s.\n",file_path);
                return;
            }

            for(int i = 0; i < stl.num_triangles; ++i)
            {
                if(is_vec3f_unique(unique_vertices,vertex_count,&stl.triangles[i].vertex1, &matched_index))
                {
                    copy_v3f(&unique_vertices[vertex_count++],&stl.triangles[i].vertex1);
                    indices[index_count++] = vertex_count-1;
                }
                else
                    indices[index_count++] = matched_index;

                if(is_vec3f_unique(unique_vertices,vertex_count,&stl.triangles[i].vertex2, &matched_index))
                {
                    copy_v3f(&unique_vertices[vertex_count++],&stl.triangles[i].vertex2);
                    indices[index_count++] = vertex_count-1;
                }
                else
                    indices[index_count++] = matched_index;

                if(is_vec3f_unique(unique_vertices,vertex_count,&stl.triangles[i].vertex3, &matched_index))
                {
                    copy_v3f(&unique_vertices[vertex_count++],&stl.triangles[i].vertex3);
                    indices[index_count++] = vertex_count-1;
                }
                else
                    indices[index_count++] = matched_index;
            }
        }
        break;
        case MODEL_FORMAT_OBJ:
            printf("OBJ file support in development.\n");
            return;
        default:
            printf("Unsupposed model format.\n");
            return;
    }

    // copy values over to mesh
    mesh->num_vertices = vertex_count;
    mesh->vertices = malloc(vertex_count*sizeof(Vertex));
    memset(mesh->vertices,0,vertex_count*sizeof(Vertex));

    for(int i = 0; i < vertex_count; ++i)
    {
        copy_v3f(&mesh->vertices[i].position, &unique_vertices[i]);
        mesh->vertices[i].tex_coord.x = 0.0f;
        mesh->vertices[i].tex_coord.y = 1.0f;
    }

    mesh->num_indices = index_count;
    mesh->indices = malloc(index_count*sizeof(u32));
    memcpy(mesh->indices,indices,index_count*sizeof(u32));

    print_mesh(mesh);
}

void mesh_render(Mesh* mesh)
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->ibo);

    if(mesh->mat.texture.data)
        texture_bind(&mesh->mat.texture,GL_TEXTURE0);

    if(show_wireframe)
        glDrawElements(GL_LINES, mesh->num_indices, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    texture_unbind();

}
