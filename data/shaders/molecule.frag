uniform sampler2D bg_texture;

uniform vec4 color;
uniform vec3 light_pos;
uniform vec3 camera_pos;

varying vec3 world_pos;
varying vec3 world_normal;

float PI = 3.14159;

void main(void)
{
//    vec3 vec_to_light = light_pos - world_pos;
    vec3 vec_to_light = normalize(vec3(0.5, -0.5, 1.0));
    float lambert = dot(normalize(vec_to_light), world_normal);

    vec3 vec_to_camera = normalize(camera_pos - world_pos);
    vec3 view_dir = normalize(world_pos - camera_pos);

    vec3 ref_dir = normalize(2.0 * dot(vec_to_light, world_normal) * world_normal - vec_to_light);

//    vec3 ref_view_dir = normalize(vec_to_camera - 2.0 * dot(vec_to_camera, world_normal) * world_normal);
    vec3 ref_view_dir = reflect(view_dir, normalize(world_normal));

    float gamma = 8.0;
    float alpha = 10.0;
    float beta = alpha / gamma;

//    float lambda = length(ref_dir - vec_to_camera) * 0.5;
    float lambda = 1.0 - dot(ref_dir, vec_to_camera);

//    float spec = pow(max(0.0, 1.0 - beta * lambda), gamma);
    float spec = pow(max(0.0, 1.0 - lambda), alpha);

    float fresnel = min(1.0, dot(normalize(vec_to_camera), world_normal) * 0.5 + 0.5);

    float theta = acos(ref_view_dir.z);
    float phi = atan(ref_view_dir.y, ref_view_dir.x);

    vec4 bg_color = texture2D(bg_texture, vec2((phi + PI) / (2.0 * PI), theta / PI));
//    vec4 bg_color = vec4((phi + 3.14159) / (2.0 * 3.14159), theta / 3.14159, 0.0, 1.0);
//    vec4 bg_color = vec4((phi + 3.14159) / (2.0 * 3.14159), 0.0, 0.0, 1.0);


//    gl_FragColor = vec4(color.rgb * lambert, 1.0);
//    gl_FragColor = vec4(color.rgb * fresnel, 1.0);
//    gl_FragColor = vec4(bg_color.rgb, 1.0);
    gl_FragColor = vec4((bg_color.rgb * (1.0 - fresnel)) + max(0.2, lambert) * fresnel * color.rgb + vec3(spec), 1.0);
//    gl_FragColor = vec4(bg_color.rgb * (1.0 - fresnel) + fresnel * color.rgb, 1.0);

//    gl_FragColor = vec4(vec3(fresnel), 1.0);

//    gl_FragColor = vec4(vec3(lambda), 1.0);
//    gl_FragColor = vec4(ref_view_dir, 1.0);


//    gl_FragColor = vec4(world_normal.xyz, 1.0);
}
