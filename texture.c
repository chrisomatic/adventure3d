#include <GL/gl.h>
#include <stdbool.h>
#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "util/stb_image.h"

bool texture_load(Texture* t, const char* filepath)
{
    int x,y,n;
    t->data = stbi_load(filepath, &x, &y, &n, 0);

    if(!t->data)
    {
        printf("Failed to load file (%s)",filepath);
        return false;
    }
    
    printf("Loaded file %s. w: %d h: %d\n",filepath,x,y);
    
    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, t->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(t->data);
    return true;
}

void texture_bind(Texture* t, GLenum texture_unit)
{
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, t->id);
}

void texture_unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
