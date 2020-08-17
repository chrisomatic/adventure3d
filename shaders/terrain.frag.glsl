#version 330 core

in vec4 color;
in vec2 tex_coord0;
in vec3 normal0;
in vec3 vertex_position;

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
uniform int wireframe;
uniform vec3 camera_position;

void main()
{
    if(wireframe == 1)
    {
        frag_color = vec4(0.2f,1.0f,0.2f,1.0f);
    }
    else
    {
        vec4  ambient_color  = vec4(dl.color * dl.ambient_intensity, 1.0f);
        float diffuse_factor = dot(normalize(normal0), -dl.direction);

        //float dist    = distance(vertex_position,camera_position);
        //float opacity = clamp(dist/1000.0, 0, 1);

        vec4 diffuse_color;

        if (diffuse_factor > 0)
        {
            diffuse_color = vec4(dl.color * dl.diffuse_intensity * diffuse_factor, 1.0f);
        }
        else
        {
            diffuse_color = vec4(0.0, 0.0, 0.0, 0.0);
        }

        //if(opacity > 0.25 && opacity < 1.0)
        //    diffuse_color = vec4(1.0,0.0,0.0,0.0);

        frag_color = texture(sampler, tex_coord0.xy) * (ambient_color + diffuse_color); //* vec4(1.0, 1.0, 1.0, 1.0 - opacity);
    }
}
