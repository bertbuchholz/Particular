#version 330

uniform sampler2D pos_tex;
uniform sampler2D charge_tex;
uniform sampler2D radius_tex;
uniform sampler2D parent_id_tex;

//in vec2 texc;

//uniform float scale;
//uniform vec2 offset;
uniform vec2 tex_size;
uniform int num_atoms;

layout(location = 0) out vec4 out_force;

const float coulomb_strength = 155.0;
const float vdw_strength = 2.0;
const float vdw_radius_factor = 1.4;

float calc_coulomb_force(float distance, float charge_0, float charge_1)
{
    return coulomb_strength * charge_0 * charge_1 / (distance * distance);
}

float calc_van_der_waals_force(float distance, float radius_0, float radius_1)
{
    float vdw_radii = radius_0 + radius_1;
    float sigma = vdw_radius_factor * vdw_radii;
    float sigma_distance = sigma / distance;
    float pow_6 = sigma_distance * sigma_distance * sigma_distance * sigma_distance * sigma_distance * sigma_distance;
    return vdw_strength * 4.0f * (pow_6 * pow_6 - pow_6);
}

void main(void)
{
//    out_force = vec4(-10.0, 0.0, 0.0, 1.0);
//    return;

//    int receiving_atom_index = int(gl_FragCoord.x) + tex_size.x * int(gl_FragCoord.y);

    vec2 receiver_frag_coord = gl_FragCoord.xy / tex_size;

    vec3 pos_receiver = texture2D(pos_tex, receiver_frag_coord).xyz;
    float charge_receiver = texture2D(charge_tex, receiver_frag_coord).x;
    float radius_receiver = texture2D(radius_tex, receiver_frag_coord).x;
    int receiver_parent_id = int(texture2D(parent_id_tex, receiver_frag_coord).x);
//    uint receiver_parent_id = texture(parent_id_tex, receiver_frag_coord).x;

    vec3 force = vec3(0.0, 0.0, 0.0);

    int debug_num_encountered_atoms = 0;

    for (int i = 0; i < num_atoms; ++i)
    {
//        if (i == receiving_atom_index) continue;

        vec2 sender_frag_coord = vec2(i % int(tex_size.x) + 0.5, i / int(tex_size.x) + 0.5);
        sender_frag_coord /= tex_size;

        int sender_parent_id = int(texture2D(parent_id_tex, sender_frag_coord).x);

        if (sender_parent_id == receiver_parent_id) continue;

//        out_force = vec4(1.0 * float(sender_parent_id), 0.0, 0.0, 1.0);
//        out_force = vec4(sender_frag_coord, 0.0, 1.0);
//        return;
        ++debug_num_encountered_atoms;

        vec3 pos_sender = texture2D(pos_tex, sender_frag_coord).xyz;
        float charge_sender = texture2D(charge_tex, sender_frag_coord).x;
        float radius_sender = texture2D(radius_tex, sender_frag_coord).x;

        vec3 direction = pos_receiver - pos_sender;
        float distance = length(direction);
        direction = normalize(direction);

//        force += direction * calc_coulomb_force(distance, charge_sender, charge_receiver);
        force += direction * calc_van_der_waals_force(distance, radius_sender, radius_receiver);
    }

//    out_force = vec4(gl_FragCoord.x, 0.0, 0.0, 1.0);
//    out_force = vec4(20.0 * charge_receiver, 0.0, 0.0, 1.0);
//    out_force = vec4(10.0, 0.0, 0.0, 1.0);
//    out_force = vec4(1.0 * float(receiver_parent_id), 0.0, 0.0, 1.0);
//    out_force = vec4(float(receiving_atom_index), 0.0, 0.0, 1.0);
//    out_force = vec4(float(debug_num_encountered_atoms), 0.0, 0.0, 1.0);
    out_force = vec4(force, 1.0);
}
