#version 330 core

layout(location = 0) out vec4 outColour;

uniform vec3 entityIdColor;

void main()
{
    outColour = vec4(entityIdColor, 1.0);
}
