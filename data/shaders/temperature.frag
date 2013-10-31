uniform sampler2D ice_texture;
uniform sampler2D scene_texture;

uniform float time;
uniform vec2 screen_size;

vec3 screen(vec3 base, vec3 blend)
{
    return vec3(1.0) - ((vec3(1.0) - base) * (vec3(1.0) - blend));
}

float sign(float x)
{
    return x < 0.0 ? -1.0 : 1.0;
}

void main(void)
{
    vec2 tex_coord = gl_TexCoord[0].st;

    vec4 color;
//    vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

//    color = vec4(gl_FragCoord.x / screen_size.x, gl_FragCoord.y / screen_size.y, 0.0, 1.0);
//    gl_FragColor = color;
//    return;
//    vec4 color = vec4(tex_coord.s, tex_coord.t, 0.0, 1.0);
    vec2 screen_coords = vec2(gl_FragCoord.x / screen_size.x, gl_FragCoord.y / screen_size.y);

//    color = texture2D(scene_texture, screen_coords);
//    color.a = 1.0;
//    gl_FragColor = color;
//    return;

    float mix_factor = gl_Color.x;

    // if < 0.5 -> ice, > 0.5 -> heat

    if (mix_factor < 0.5)
    {
        float f = 2.0 * (0.5 - mix_factor); // f in [0 .. 1.0]

        vec2 ice_tex_size = vec2(498.0, 329.0); // hard coded texture size :(

        float tex_value = length(texture2D(ice_texture, tex_coord).rgb);
        float dx = length(texture2D(ice_texture, tex_coord + vec2(1.0 / ice_tex_size.x, 0.0)).rgb) - tex_value;
        float dy = length(texture2D(ice_texture, tex_coord + vec2(0.0, 1.0 / ice_tex_size.y)).rgb) - tex_value;

        float refraction_strength = 0.02;
        vec2 refracted_coord = vec2(dx, dy) * f * refraction_strength;

        vec4 scene_color = texture2D(scene_texture, screen_coords + refracted_coord);
        scene_color.a = 1.0;

        vec4 tint = vec4(1.0, 1.0, 1.0, 1.0) * (1.0 - f) + vec4(0.6, 0.8, 1.0, 1.0) * f * 1.2;

//        color = ice_color * f + scene_color * (1.0 - f);
        color = scene_color * tint;
    }
    else if (mix_factor > 0.5)
    {
        float f = 2.0 * (mix_factor - 0.5);
//        f = 1.0;
//        color = vec4(1.0, 0.0, 0.0, 1.0) * f + color * (1.0 - f);

//        float refraction = 0.5 + 0.5 * sin(tex_coord.x + 200.0 * f); // + time * 10.0 * f);
        float refraction_strength = 0.002;
        float refraction = sin(200.0 * tex_coord.x + time * 10.0) * refraction_strength * f;
        float refraction2 = sin(200.0 * tex_coord.y + time * 10.0) * refraction_strength * f;

        vec2 refracted_coord = vec2(screen_coords.x + refraction, screen_coords.y + refraction2);

        color = texture2D(scene_texture, refracted_coord); // + color * (1.0 - f);
        color.a = 1.0;

        vec4 tint = vec4(1.0, 0.1, 0.1, f * 0.5) * (1.0 - f) + vec4(1.0, 0.7, 0.1, f * 0.5) * f;

        color = vec4(color.rgb * (1.0 - tint.a) + tint.rgb * tint.a, 1.0);
//        color = vec4(color.rgb * (1.0 - sqrt(tint.a)) + screen(color.rgb, tint.rgb) * sqrt(tint.a), 1.0);
    }
    else
    {
        color = texture2D(scene_texture, screen_coords);
        color.a = 1.0;
    }

    gl_FragColor = color;
}
