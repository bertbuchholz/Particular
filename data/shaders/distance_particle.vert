attribute float particle_size;
uniform float height;

void main(void)
{
    gl_Position = ftransform();
    float distance = length(gl_Position.xyz);
    gl_PointSize = 2000.0 * particle_size * height / (distance * distance);
    gl_FrontColor = gl_Color;
}
