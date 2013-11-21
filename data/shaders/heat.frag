uniform sampler2D scene_texture;

uniform float time;
uniform vec2 screen_size;

uniform float repetition_ratio;
uniform float alpha;

vec3 screen(vec3 base, vec3 blend)
{
    return vec3(1.0) - ((vec3(1.0) - base) * (vec3(1.0) - blend));
}

float brightness(vec3 color)
{
    return (color.r + color.g + color.b) * 0.3333;
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
//    vec2 screen_coords = vec2(gl_FragCoord.x / screen_size.x, gl_FragCoord.y / screen_size.y);

//    color = texture2D(scene_texture, tex_coord);
//    color.a = 1.0;
//    gl_FragColor = color;
//    return;

    float f = 1.0;

    float refraction_strength = 0.02;

//    float refraction = sin(10.0 * repetition_ratio * tex_coord.x + time * 5.0) * refraction_strength * f;
//    float refraction2 = sin(20.0 * tex_coord.y - time * 5.0) * refraction_strength * f;

    float refraction = 0.0;
    float refraction2 = sin(20.0 * tex_coord.y - time * 5.0 - sin(tex_coord.x * 80.0)) * refraction_strength * f;


    vec2 refracted_coord = vec2(tex_coord.x + refraction, tex_coord.y + refraction2);

    color = texture2D(scene_texture, refracted_coord); // + color * (1.0 - f);
//    color.a = 1.0;

    vec4 tint = vec4(1.0, 0.7, 0.1, f * 0.5);

//    color = vec4(color.rgb * (1.0 - tint.a) + tint.rgb * tint.a, color.a);
//    color.rgb = color.rgb * (1.0 - tint.a) + tint.rgb * tint.a;
//    color.rgb = screen(color.rgb, tint.rgb) * 0.4 + color.rgb * 0.6;
//    color.rgb = tint.rgb * 0.4 + color.rgb * 0.6;
    color.rgb = tint.rgb * brightness(color.rgb) * 0.2 + color.rgb * 0.8;



    color.a = color.a * alpha;

//    color = vec4(refracted_coord.s, refracted_coord.t, 0.0, color.a);
//    color = vec4(tex_coord.s, tex_coord.t, 0.0, 1.0);
//    color.a = 1.0;

    gl_FragColor = color;
}
