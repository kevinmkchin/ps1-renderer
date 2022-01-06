#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 vertex_uv;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    gl_Position =  projMatrix * viewMatrix * modelMatrix * vec4(vertex_pos, 1.0);
}
