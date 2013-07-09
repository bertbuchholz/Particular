uniform sampler2D ice_texture;
uniform sampler2D scene_texture;

uniform float time;
uniform vec2 screen_size;

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

    color = texture2D(scene_texture, screen_coords);
//    gl_FragColor = color;
//    return;

    float mix_factor = gl_Color.x;

    // if < 0.5 -> ice, > 0.5 -> heat

    if (mix_factor < 0.5)
    {
        float f = 1.2 * (0.5 - mix_factor);
        color = texture2D(ice_texture, tex_coord) * f + color * (1.0 - f);
//        color = texture2D(ice_texture, tex_coord);
//        color.a = f;
    }
    else if (mix_factor > 0.5)
    {
        float f = 2.0 * (mix_factor - 0.5);
//        f = 1.0;
//        color = vec4(1.0, 0.0, 0.0, 1.0) * f + color * (1.0 - f);

//        float refraction = 0.5 + 0.5 * sin(tex_coord.x + 200.0 * f); // + time * 10.0 * f);
        float refraction = sin(200.0 * tex_coord.x + time * 10.0) * 0.002 * f;
        float refraction2 = sin(200.0 * tex_coord.y + time * 10.0) * 0.002 * f;

        vec2 refracted_coord = vec2(screen_coords.x + refraction, screen_coords.y + refraction2);

        color = texture2D(scene_texture, refracted_coord); // + color * (1.0 - f);

        vec4 tint = vec4(1.0, 0.1, 0.1, f * 0.5) * (1.0 - f) + vec4(1.0, 0.7, 0.1, f * 0.5) * f;

        color = vec4(color.rgb * (1.0 - tint.a) + tint.rgb * tint.a, 1.0);
    }
    else
    {
//        color.a = 0.0;
        color = texture2D(scene_texture, screen_coords);
    }

//    vec4 color = gl_Color;

    gl_FragColor = color;
}
