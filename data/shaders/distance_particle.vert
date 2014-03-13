attribute float particle_size;
uniform float height;

void main(void)
{
    gl_Position = ftransform();
    float distance = length(gl_Position.xyz);
    gl_PointSize = max(2.0, min(32.0, 2000.0 * particle_size * height / (distance * distance)));
    gl_FrontColor = gl_Color;
}
