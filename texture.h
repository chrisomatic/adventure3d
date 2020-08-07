#pragma once

#include <stdbool.h>

extern GLuint texture1;
extern GLuint texture2;
extern GLuint texture_cube;

void texture_load_all();
bool texture_load2d(GLuint* t, const char* filepath);
void texture_bind(GLuint* t, GLenum texture_unit);
void texture_unbind();
