#version 330

uniform sampler2D pos_tex;
uniform sampler2D charge_tex;

//in vec2 texc;

//uniform float scale;
//uniform vec2 offset;
uniform vec2 tex_size;
uniform int num_atoms;

layout(location = 0) out vec4 out_force;

const float coulomb_strength = 1.0;

float calc_coulomb_force(float distance, float charge_0, float charge_1)
{
    return coulomb_strength * charge_0 * charge_1 / (distance * distance);
}

void main(void)
{
    int receiving_atom_index = int(gl_FragCoord.x + tex_size.x * gl_FragCoord.y);

    vec3 pos_receiver = texture2D(pos_tex, gl_FragCoord.xy / tex_size).xyz;

    float charge_receiver = texture2D(charge_tex, gl_FragCoord.xy / tex_size).x;

    vec3 force = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < num_atoms; ++i)
    {
        if (i == receiving_atom_index) continue;

        vec2 sender_frag_coord = vec2(i % int(tex_size.x), i / int(tex_size.x));
        sender_frag_coord /= tex_size;

        vec3 pos_sender = texture2D(pos_tex, sender_frag_coord).xyz;
        float charge_sender = texture2D(charge_tex, sender_frag_coord).x;

        vec3 direction = pos_receiver - pos_sender;
        float distance = length(direction);
        direction = normalize(direction);

        force += direction * calc_coulomb_force(distance, charge_sender, charge_receiver);
    }

//    vec2 coords = texc.st;
//    coords += offset;

//    coords *= scale;

//    bool mod_x = coords.x - floor(coords.x) > 0.5;
//    bool mod_y = coords.y - floor(coords.y) > 0.5;

//    float color = (mod_x == mod_y) ? 1.0 : 0.0;

    out_force = vec4(1.0, 1.0, 0.0, 1.0);
//    out_force = force;
}
