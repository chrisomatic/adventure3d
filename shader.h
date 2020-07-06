#pragma once

#define MAX_SHADER_LEN 1024
#define INVALID_UNIFORM_LOCATION 0xFFFFFFFF

typedef struct {
    GLuint color;
    GLuint ambient_intensity;
    GLuint diffuse_intensity;
    GLuint direction;
} DirLightLocation;

extern GLuint program;

extern GLuint world_location;
extern GLuint wvp_location;
extern GLuint sampler;
extern DirLightLocation dir_light_location;

void shader_load_all();
void shader_deinit();
