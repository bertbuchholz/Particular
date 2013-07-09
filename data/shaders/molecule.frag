uniform vec4 color;
uniform vec3 light_pos;
uniform vec3 camera_pos;

varying vec3 world_pos;
varying vec3 world_normal;

void main(void)
{
    vec3 vec_to_light = light_pos - world_pos;
    float lambert = dot(normalize(vec_to_light), world_normal);

    vec3 vec_to_camera = camera_pos - world_pos;
    float fresnel = dot(normalize(vec_to_camera), world_normal);


//    gl_FragColor = vec4(color.rgb * lambert, 1.0);
    gl_FragColor = vec4(color.rgb * fresnel, 1.0);

//    gl_FragColor = vec4(world_pos.xyz, 1.0);
}
