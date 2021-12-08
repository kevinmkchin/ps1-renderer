#version 330 core

in vec3 vary_pos;
in vec2 vary_uv;
in vec3 LIGHT;

layout(location = 0) out vec4 out_colour;

uniform sampler2D texture0;

void main()
{
    vec3 COLOR = texture(texture0, vary_uv).xyz;
    vec3 TOTAL = COLOR * LIGHT;
    out_colour = vec4(TOTAL, 1.0);
}
