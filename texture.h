#pragma once

typedef struct
{
    GLuint id;
    unsigned char *data;
} Texture;

extern Texture texture1;
extern Texture texture2;

void texture_load_all();
bool texture_load(Texture* t, const char* filepath);
void texture_bind(Texture* t, GLenum texture_unit);
void texture_unbind();
