#version 330

uniform mat4 m_view;
uniform mat4 m_model;
uniform mat4 m_projection;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec3 in_color;

out vec3 world_pos;
out vec3 world_normal;

void main(void)
{
    world_pos = vec4(m_model * vec4(in_pos, 1.0)).xyz;
    world_normal = normalize(vec4(m_model * vec4(in_normal, 0.0)).xyz);

    gl_Position = m_projection * m_view * m_model * vec4(in_pos, 1.0);
}
