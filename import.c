#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "util.h"
#include "math3d.h"
#include "import.h"

/*

STL BINARY FILE FORMAT

    UINT8[80] – Header
    UINT32 – Number of triangles

    foreach triangle
    REAL32[3] – Normal vector
    REAL32[3] – Vertex 1
    REAL32[3] – Vertex 2
    REAL32[3] – Vertex 3
    UINT16 – Attribute byte count
    end
   
*/

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

static void print_stl(STL* stl)
{
    printf("\n");
    printf("Header: %s\n",stl->header);
    printf("Num Triangles: %d\n",stl->num_triangles);

    for(int i = 0; i < stl->num_triangles; ++i)
    {
        printf("Triangle%d: %f %f %f %f, %u\n", stl->triangles[i].normal, stl->triangles[i].vertex1, stl->triangles[i].vertex2, stl->triangles[i].vertex3, stl->triangles[i].attr_count);
    }
    printf("\n");
}

bool import_stlb(const char* file_path)
{
    FILE* fp = fopen(file_path,"rb");

    if(!fp)
    {
        fprintf(stderr,"Failed to open file %s\n",file_path);
        return false;
    }

    STL stl = {0};

    // read header
    fread(stl.header, 80, sizeof(u8),fp);

    // read num triangles
    fread(&stl.num_triangles,1, sizeof(u32),fp);

    // allocate space to triangles
    stl.triangles = malloc(stl.num_triangles*sizeof(STL_Triangle));

    if(!stl.triangles)
    {
        fprintf(stderr,"Failed to allocate memory for STL triangles.\n");
        return false;
    }

    // read triangles
    for(int i = 0; i < stl.num_triangles; ++i)
    {
        fread(&stl.triangles[i].normal,    3, sizeof(float),fp);
        fread(&stl.triangles[i].vertex1,   3, sizeof(float),fp);
        fread(&stl.triangles[i].vertex2,   3, sizeof(float),fp);
        fread(&stl.triangles[i].vertex3,   3, sizeof(float),fp);
        fread(&stl.triangles[i].attr_count,3, sizeof(u16)  ,fp);
    }

    print_stl(&stl);

    fclose(fp);

    return true;
}
