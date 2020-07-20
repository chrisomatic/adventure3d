#pragma once

#include "math3d.h"

#define MAX_SHADER_LEN 2048
#define INVALID_UNIFORM_LOCATION 0xFFFFFFFF

typedef struct {
    GLuint color;
    GLuint ambient_intensity;
    GLuint diffuse_intensity;
    GLuint direction;
} DirLightLocation;

extern GLuint program;
extern GLuint sky_program;

extern GLuint world_location;
extern GLuint wvp_location;
extern GLuint sampler;
extern GLuint wireframe_location;
extern DirLightLocation dir_light_location;
extern GLuint camera_position_location;

void shader_load_all();
void shader_deinit();

void shader_set_int(GLuint program, const char* name, int i);
void shader_set_float(GLuint program, const char* name, float f);
void shader_set_vec3(GLuint program, const char* name, float x, float y, float z);
void shader_set_mat4(GLuint program, const char* name, Matrix4f* m);
