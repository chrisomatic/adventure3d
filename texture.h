#pragma once

typedef struct
{
    GLuint id;
    unsigned char *data;
} Texture;

bool texture_load(Texture* t, const char* filepath);
void texture_bind(Texture* t, GLenum texture_unit);
void texture_unbind();
