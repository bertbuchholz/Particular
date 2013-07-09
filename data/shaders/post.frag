uniform sampler2D texture;
uniform sampler2D depth_texture;

uniform vec2 clip_distances;

uniform vec2 tex_size;

float linearize_depth(float zoverw)
{
        float n = clip_distances.s; // You had to change this
        float f = clip_distances.t; // and this

        return (2.0 * n) / (f + n - zoverw * (f - n));
}

vec3 blur(vec2 pos, float amount)
{
    int resolution = int(amount * 6.0) / 2;

    vec3 result = vec3(0.0, 0.0, 0.0);

    for (int u = -resolution; u <= resolution; ++u)
    {
        for (int v = -resolution; v <= resolution; ++v)
        {
            result += texture2D(texture, pos + vec2(float(u) / tex_size.s, float(v) / tex_size.t)).rgb;
//            result += texture2D(texture, pos).rgb;
        }
    }

    return result / float((2 * resolution + 1) * (2 * resolution + 1));
}

void main(void)
{
    vec2 tex_coord = gl_TexCoord[0].st;

//    out_color = vec4(1.0);
//    vec4 color = vec4(gl_FragCoord.x, gl_FragCoord.y, 0.0, 1.0);
//    vec4 color = vec4(tex_coord.s, tex_coord.t, 0.0, 1.0);
    vec4 color = texture2D(texture, tex_coord);

    float depth = texture2D(depth_texture, tex_coord).r;
    depth = linearize_depth(depth) * 2.0;

//    color = vec4(vec3(depth), 1.0);

    color = vec4(blur(tex_coord, depth), 1.0);

    gl_FragColor = color;
}
