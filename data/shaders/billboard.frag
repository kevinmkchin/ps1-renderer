#version 330 core

in vec2 vary_uv;

out vec4 colour;

uniform sampler2D quadTexture;
uniform vec3 quadColor;

void main()
{
    if(quadColor.xyz == vec3(0.0))
    {
        colour = texture(quadTexture, vary_uv);
    }
    else
    {
        colour = vec4(quadColor, 1.0);
    }
}
