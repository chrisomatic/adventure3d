#version 330 core

in vec2 tex_coord0;
in vec3 normal0;

out vec4 frag_color;

struct DirectionalLight
{
    vec3  color;
    float ambient_intensity;
    float diffuse_intensity;
    vec3  direction;
};

uniform DirectionalLight dl;
uniform sampler2D sampler;

void main()
{
    vec4 ambient_color   = vec4(dl.color * dl.ambient_intensity, 1.0f);
    float diffuse_factor = dot(normalize(normal0), -dl.direction);

    vec4 diffuse_color;

    if (diffuse_factor > 0)
    {
        diffuse_color = vec4(dl.color * dl.diffuse_intensity * diffuse_factor, 1.0f);
    }
    else
    {
        diffuse_color = vec4(0, 0, 0, 0);
    }

    frag_color = texture2D(sampler, tex_coord0.xy) *
                 (ambient_color + diffuse_color);
};
