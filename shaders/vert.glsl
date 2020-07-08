#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 tex_coord;
layout (location = 2) in vec3 normal;

uniform mat4 wvp;
uniform mat4 world;

out vec2 tex_coord0;
out vec3 normal0;

void main()
{
    gl_Position = wvp * vec4(position, 1.0);
    tex_coord0 = tex_coord;
    normal0 = (world * vec4(normal, 0.0)).xyz;
    //color = vec4(clamp(position, 0.0, 1.0), 1.0);
};
