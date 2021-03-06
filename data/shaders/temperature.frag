#version 330

uniform sampler2D ice_texture;
uniform sampler2D scene_texture;

uniform float time;
uniform vec2 screen_size;

in vec3 world_pos;
in vec3 world_normal;
in vec2 uv;
in vec3 color;

out vec4 out_color;

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
    out_color.a = 1.0;
//    out_color = vec4(uv, 0.0, 1.0);
//    out_color = vec4(1.0, 0.0, 0.0, 1.0);
//    return;

    vec2 tex_coord = uv;

    vec2 screen_coords = gl_FragCoord.xy / screen_size;

    float mix_factor = color.x;

    // if < 0.5 -> ice, > 0.5 -> heat

    if (mix_factor < 0.5)
    {
        float f = 2.0 * (0.5 - mix_factor); // f in [0 .. 1.0]

        vec2 ice_tex_size = vec2(498.0, 329.0); // hard coded texture size :(

        float tex_value = length(texture2D(ice_texture, tex_coord).rgb);
        float dx = length(texture2D(ice_texture, tex_coord + vec2(1.0 / ice_tex_size.x, 0.0)).rgb) - tex_value;
        float dy = length(texture2D(ice_texture, tex_coord + vec2(0.0, 1.0 / ice_tex_size.y)).rgb) - tex_value;

        float refraction_strength = 0.035;
        vec2 refracted_coord = vec2(dx, dy) * f * refraction_strength;

        vec4 scene_color = texture2D(scene_texture, screen_coords + refracted_coord);
        scene_color.a = 1.0;

        tex_value = tex_value / 1.71;
        vec3 tint = vec3(1.0, 1.0, 1.0) * (1.0 - f) + vec3(0.8, 0.84, 0.87) * f * (tex_value * 0.2 + 0.8);


//        color = ice_color * f + scene_color * (1.0 - f);
        out_color.rgb = scene_color.rgb * tint.rgb; // * tex_value;
//        color = scene_color * tex_value;

//        color = vec4(tex_value, tex_value, tex_value, 1.0);
//        color = vec4(texture2D(ice_texture, tex_coord).rgb, 1.0);
//        color = vec4(texture2D(scene_texture, screen_coords).rgb, 1.0) * 0.7;
//        color = vec4(screen_coords.s, screen_coords.t, 0.0, 1.0);
    }
    else if (mix_factor > 0.5)
    {
        float f = 2.0 * (mix_factor - 0.5);
//        f = 1.0;
//        color = vec4(1.0, 0.0, 0.0, 1.0) * f + color * (1.0 - f);

        float refraction_strength = 0.002 * f;

//        float refraction = sin(75.0 * tex_coord.x + time * 15.0 * f) * refraction_strength;
//        float refraction2 = sin(75.0 * tex_coord.y + time * 15.0 * f) * refraction_strength;

        vec2 refraction = vec2(0.0,
                               sin(20.0 * tex_coord.y - time * 5.0 - sin(tex_coord.x * 80.0)) * refraction_strength * f);

//        float refraction_strength = 0.002;
//        float refraction = sin(200.0 * tex_coord.x + time * 10.0) * refraction_strength * f;
//        float refraction2 = sin(200.0 * tex_coord.y + time * 10.0) * refraction_strength * f;

        vec2 refracted_coord = screen_coords + refraction; // vec2(screen_coords.x + refraction, screen_coords.y + refraction2);

        out_color = texture2D(scene_texture, refracted_coord); // + color * (1.0 - f);
        out_color.a = 1.0;

        vec4 tint = vec4(1.0, 0.1, 0.1, f * 0.5) * (1.0 - f) + vec4(1.0, 0.7, 0.1, f * 0.5) * f;

        out_color = vec4(out_color.rgb * (1.0 - tint.a) + tint.rgb * tint.a, 1.0);
//        color = vec4(color.rgb * (1.0 - sqrt(tint.a)) + screen(color.rgb, tint.rgb) * sqrt(tint.a), 1.0);
    }
    else
    {
        out_color = texture2D(scene_texture, screen_coords);
        out_color.a = 1.0;
    }
}
