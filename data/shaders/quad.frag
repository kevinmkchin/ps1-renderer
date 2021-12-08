#version 330 core

in vec2 vary_uv;

out vec4 colour;

uniform sampler2D quadTexture;

void main()
{
    colour = texture(quadTexture, vary_uv);
}