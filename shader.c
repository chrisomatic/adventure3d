#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "util.h"
#include "shader.h"

static void shader_add(GLuint program, GLenum shader_type, const char* shader_file_path);

GLuint program;

GLuint world_location;
GLuint wvp_location;
GLuint sampler;

DirLightLocation dir_light_location;

void shader_load_all()
{
	program = glCreateProgram();

    shader_add(program, GL_VERTEX_SHADER,  "shaders/vert.glsl");
    shader_add(program, GL_FRAGMENT_SHADER,"shaders/frag.glsl");

	glLinkProgram(program);

	GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
    {
        GLchar info[1000+1] = {0};
		glGetProgramInfoLog(program, 1000, NULL, info);
		fprintf(stderr, "Error linking shader program: '%s'\n", info);
        exit(1);
	}

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        GLchar info[1000+1] = {0};
        glGetProgramInfoLog(program, 1000, NULL, info);
        fprintf(stderr, "Invalid shader program: '%s'\n", info);
        exit(1);
    }

    glUseProgram(program);

    // Get uniform locations
    world_location = glGetUniformLocation(program,"world");
    wvp_location   = glGetUniformLocation(program,"wvp");

    sampler = glGetUniformLocation(program, "sampler");

    dir_light_location.color             = glGetUniformLocation(program, "dl.color");
    dir_light_location.ambient_intensity = glGetUniformLocation(program, "dl.ambient_intensity");
    dir_light_location.diffuse_intensity = glGetUniformLocation(program, "dl.diffuse_intensity");
    dir_light_location.direction         = glGetUniformLocation(program, "dl.direction");

    if(world_location                       == INVALID_UNIFORM_LOCATION ||
       sampler                              == INVALID_UNIFORM_LOCATION ||
       dir_light_location.color             == INVALID_UNIFORM_LOCATION ||
       dir_light_location.ambient_intensity == INVALID_UNIFORM_LOCATION ||
       dir_light_location.diffuse_intensity == INVALID_UNIFORM_LOCATION ||
       dir_light_location.direction         == INVALID_UNIFORM_LOCATION
      ) {
        fprintf(stderr,"Failed to find all shader uniform locations.\n");
    }
}

void shader_deinit()
{
    glDeleteProgram(program);
}

static void shader_add(GLuint program, GLenum shader_type, const char* shader_file_path)
{
    // create
	GLuint shader_id = glCreateShader(shader_type);
    if (!shader_id)
    {
        fprintf(stderr, "Error creating shader type %d\n", shader_type);
        exit(1);
    }

    char* buf = calloc(MAX_SHADER_LEN+1,sizeof(char));
    if(!buf)
    {
        fprintf(stderr, "Failed to malloc shader buffer\n");
        exit(1);
    }

    // load
    int len = read_file(shader_file_path,buf, MAX_SHADER_LEN);
    if(len == 0)
    {
        printf("Read zero bytes from shader file.\n");
        return;
    }

	// compile
	printf("Compiling shader: %s (size: %d bytes)\n", shader_file_path, len);

	glShaderSource(shader_id, 1, (const char**)&buf, NULL);
	glCompileShader(shader_id);

	// validate
	GLint success;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
	if (!success)
    {
        GLchar info[1000+1] = {0};
		glGetShaderInfoLog(shader_id, 1000, NULL, info);
		fprintf(stderr,"Error compiling shader type %d: '%s'\n", shader_type, info);
        free(buf);
        exit(1);
	}

	glAttachShader(program, shader_id);

    free(buf);
}
