#version 330 core

layout(location = 0) out vec4 outColour;

uniform sampler2D texture0;
uniform vec4 wireframeColor;

void main()
{
    outColour = wireframeColor;
}
