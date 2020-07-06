#pragma once

typedef struct
{
    GLuint texture_obj;
    unsigned char *data;
} Texture;

bool texture_load(Texture* t, const char* filepath);
void texture_bind(Texture* t, GLenum texture_unit);
