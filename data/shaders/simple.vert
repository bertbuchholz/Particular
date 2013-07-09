uniform mat4 m_view;
uniform mat4 m_model;
uniform mat4 m_projection;

varying vec3 world_pos;
varying vec3 world_normal;

void main(void)
{
    world_pos = vec4(m_model * gl_Vertex).xyz;
    world_normal = vec4(m_model * vec4(gl_Normal, 0.0)).xyz;

    gl_Position = m_projection * m_view * m_model * gl_Vertex;
}
