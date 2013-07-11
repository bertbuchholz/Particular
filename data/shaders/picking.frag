#version 120

uniform vec3 color;

void main(void)
{
//    gl_FragColor = gl_Color;
    gl_FragColor = vec4(color, 1.0);
}
