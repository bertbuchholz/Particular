uniform sampler2D texture;

void main(void)
{
    vec2 tex_coord = gl_TexCoord[0].st;

//    out_color = vec4(1.0);
//    vec4 color = vec4(gl_FragCoord.x, gl_FragCoord.y, 0.0, 1.0);
//    vec4 color = vec4(tex_coord.s, tex_coord.t, 0.0, 1.0);
    vec4 color = texture2D(texture, tex_coord);

    gl_FragColor = color;
}
