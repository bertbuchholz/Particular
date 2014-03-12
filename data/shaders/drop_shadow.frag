uniform sampler2D texture;

uniform vec2 tex_size;

uniform float offset;
uniform int blur_size;

float wendland_2_1(float x)
{
    float a = 1.0 - x;
    a = a * a * a;
    return a * (3.0 * x + 1.0);
}

float blur_alpha(vec2 pos, int radius)
{
    vec2 step = 1.0 / tex_size;

    float result = 0.0;

    float weight_sum = 0.0;

    for (int u = -radius; u <= radius; ++u)
    {
        for (int v = -radius; v <= radius; ++v)
        {
            float weight = wendland_2_1(sqrt(float(u * u  + v * v)) / float(radius + 1));
            result += texture2D(texture, pos + step * vec2(u, v) - vec2(offset, -offset)).a * weight;
            weight_sum += weight;
        }
    }

    return result / weight_sum;
}

void main(void)
{
    vec2 tex_coord = gl_TexCoord[0].st;

    vec4 color = texture2D(texture, tex_coord) * gl_Color; // + color * (1.0 - f);

    float shadow = blur_alpha(tex_coord, blur_size / 2);

    color = color * color.a + vec4(0.0, 0.0, 0.0, shadow * 0.75) * (1.0 - color.a);

//    color = vec4(blur_alpha(tex_coord, blur_size), 0.0, 0.0, 1.0);


//    color = vec4(tex_coord.s, tex_coord.t, 0.0, 1.0);
//    color.a = 1.0;

    gl_FragColor = color;
}
