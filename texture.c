#include <stdio.h>

#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include "util/stb_image.h"

#include <stdbool.h>
#include "util.h"
#include "texture.h"

GLuint texture1     = {0};
GLuint texture2     = {0};
GLuint texture_cube = {0};

static bool texture_load2d(GLuint* t, const char* filepath)
{
    int x,y,n;
    unsigned char* data;  
    data = stbi_load(filepath, &x, &y, &n, 0);

    if(!data)
    {
        printf("Failed to load file (%s)",filepath);
        return false;
    }
    
    printf("Loaded file %s. w: %d h: %d\n",filepath,x,y);


    GLenum format;
    if(n == 3) format = GL_RGB;
    else       format = GL_RGBA;
    
    glGenTextures(1, t);
    glBindTexture(GL_TEXTURE_2D, *t);
    glTexImage2D(GL_TEXTURE_2D, 0, format, x, y, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    return true;
}

static bool texture_load_cube(GLuint* t, char* cube_file_paths[], int num_file_paths)
{
    glGenTextures(1, t);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *t);

    int x,y,n;
    unsigned char* data;  

    GLenum format;

    printf("Generating cubemap texture.\n");

    for(unsigned int i = 0; i < num_file_paths; i++)
    {
        data = stbi_load(cube_file_paths[i], &x, &y, &n, 0);

        if(n == 3) format = GL_RGB;
        else       format = GL_RGBA;

        if(!data)
        {
            printf("Failed to load file (%s)",cube_file_paths[i]);
            return false;
        }

        printf("Loaded file %s. w: %d h: %d\n",cube_file_paths[i],x,y);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, x, y, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 

    return true;
}

void texture_bind(GLuint* t, GLenum texture_unit)
{
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, *t);
}

void texture_unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void texture_load_all()
{
    texture_load2d(&texture1,"textures/grass.png");
    texture_load2d(&texture2,"textures/donut.png");

    char* cube[] = {
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };

    texture_load_cube(&texture_cube,cube,6);
}
