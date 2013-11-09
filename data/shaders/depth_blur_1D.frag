uniform sampler2D texture;
uniform sampler2D depth_texture;

uniform vec2 clip_distances;
uniform float focus_distance;
uniform vec2 direction;
uniform vec2 tex_size;

float wendland_2_1(float x)
{
    float a = 1.0 - x;
    a = a * a * a;
    return a * (3.0 * x + 1.0);
}

float linearize_depth(float zoverw)
{
        float n = clip_distances.s;
        float f = clip_distances.t;

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
        float weight = wendland_2_1(abs(float(u) / float(resolution + 1)));
        result += texture2D(texture, pos + step * float(u)).rgb * weight;
        weight_sum += weight;
    }

    return result / weight_sum;
}

vec4 blur_with_alpha(vec2 pos, int num_steps)
{
    int resolution = num_steps / 2;
    vec2 step = direction / tex_size;

    vec4 result = vec4(0.0, 0.0, 0.0, 0.0);

    float weight_sum = 0.0;

    for (int u = -resolution; u <= resolution; ++u)
    {
        float weight = wendland_2_1(abs(float(u) / float(resolution + 1)));
        result += texture2D(texture, pos + step * float(u)) * weight;
        weight_sum += weight;
    }

    return result / weight_sum;
}

void main(void)
{
    vec2 tex_coord = gl_TexCoord[0].st;

//    vec4 color = texture2D(texture, tex_coord);

    float normalized_depth = linearize_depth(texture2D(depth_texture, tex_coord).r);
    float normalized_focus_dist = (abs(focus_distance) - clip_distances.s) / (clip_distances.t - clip_distances.s);
//    float normalized_focus_dist = (200.0 - clip_distances.s) / (clip_distances.t - clip_distances.s);
    float blur_strength = abs(normalized_focus_dist - normalized_depth) / (1.0 - normalized_focus_dist);

//    float blur_strength = 0.0;

//    if (normalized_depth < normalized_focus_dist)
//    {
//        blur_strength = 1.0 - (normalized_depth / normalized_focus_dist);
//    }
//    else
//    {
//        blur_strength = normalized_depth / (1.0 - normalized_focus_dist);
//    }

//    color = vec4(vec3(depth), 1.0);

//    color = vec4(blur(tex_coord, int(blur_strength * 30.0)), 1.0);
    vec4 color = blur_with_alpha(tex_coord, int(blur_strength * 30.0));

    gl_FragColor = color;
//    gl_FragColor = vec4(normalized_depth, normalized_depth, normalized_depth, 1.0);
}
