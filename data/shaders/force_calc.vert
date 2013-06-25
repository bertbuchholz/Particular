#version 150

out vec2 texc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;

void main(void)
{
    gl_Position = vec4(position, 1.0);
    texc = tex_coord;
}
