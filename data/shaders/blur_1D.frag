uniform sampler2D texture;
uniform sampler2D depth_texture;

uniform vec2 clip_distances;
uniform vec2 direction;
uniform vec2 tex_size;

float linearize_depth(float zoverw)
{
        float n = clip_distances.s; // You had to change this
        float f = clip_distances.t; // and this

        return (2.0 * n) / (f + n - zoverw * (f - n));
}

vec3 blur(vec2 pos, int num_steps)
{
    int resolution = num_steps / 2;
    vec2 step = direction / tex_size;

    vec3 result = vec3(0.0, 0.0, 0.0);

    float weight_sum = 0.0;

    for (int u = -resolution; u <= resolution; ++u)
    {
        float weight = 1.0;
        result += texture2D(texture, pos + step * u).rgb * weight;
    }

    return result / weight_sum;
}

void main(void)
{
    vec2 tex_coord = gl_TexCoord[0].st;

//    out_color = vec4(1.0);
//    vec4 color = vec4(gl_FragCoord.x, gl_FragCoord.y, 0.0, 1.0);
//    vec4 color = vec4(tex_coord.s, tex_coord.t, 0.0, 1.0);
    vec4 color = texture2D(texture, tex_coord);

    float blur_strength = linearize_depth(texture2D(depth_texture, tex_coord).r);

//    color = vec4(vec3(depth), 1.0);

    color = vec4(blur(tex_coord, int(blur_strength * 10.0)), 1.0);

    gl_FragColor = color;
}
