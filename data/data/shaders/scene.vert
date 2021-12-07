#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;
// layout (location = 2) in vec3 normal;

out vec3 vary_pos;
out vec2 vary_uv;

void main()
{
    gl_Position = vec4(pos, 1.0);
}
