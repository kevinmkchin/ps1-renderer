#version 330 core

in vec2 uv;
in vec3 LIGHT;

layout(location = 0) out vec4 out_colour;

uniform sampler2D texture0;
uniform vec3 modelColor;
uniform vec3 tint;

void main()
{
    vec3 COLOR = vec3(1.0);
    if(modelColor.xyz == vec3(0.0))
    {
        COLOR = texture(texture0, uv).xyz;
    }
    else
    {
        COLOR = modelColor;
    }

    vec3 TOTAL = COLOR * LIGHT * tint;
    out_colour = vec4(TOTAL, 1.0);
}
