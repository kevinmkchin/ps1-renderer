#version 330 core

noperspective in vec2 uv;
in vec3 LIGHT;

layout(location = 0) out vec4 out_colour;

uniform sampler2D texture0;

void main()
{
    vec3 COLOR = texture(texture0, uv).xyz;
    vec3 TOTAL = COLOR * LIGHT;
    out_colour = vec4(TOTAL, 1.0);
}
