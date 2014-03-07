attribute float particle_size;
uniform float height;

void main(void)
{
    gl_Position = ftransform();
    gl_PointSize = particle_size * height;
//    gl_PointSize = 5.0f;
    gl_FrontColor = gl_Color;
}
