#version 150

in vec2 texc;

uniform float scale;
uniform vec2 offset;

out vec4 out_color;

void main(void)
{
    vec2 coords = texc.st;
    coords += offset;

    coords *= scale;

    bool mod_x = coords.x - floor(coords.x) > 0.5;
    bool mod_y = coords.y - floor(coords.y) > 0.5;

    float color = (mod_x == mod_y) ? 1.0 : 0.0;

    out_color = vec4(color, color, color, 1.0);
}
