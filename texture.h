#pragma once

extern GLuint texture1;
extern GLuint texture2;
extern GLuint texture_cube;

void texture_load_all();
void texture_bind(GLuint* t, GLenum texture_unit);
void texture_unbind();
