uniform sampler2D texture;

uniform float blur_strength;
uniform vec2 direction;
uniform vec2 tex_size;

float wendland_2_1(float x)
{
    float a = 1.0 - x;
    a = a * a * a;
    return a * (3.0 * x + 1.0);
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

    vec4 color = blur_with_alpha(tex_coord, int(blur_strength));

    gl_FragColor = color;
}
