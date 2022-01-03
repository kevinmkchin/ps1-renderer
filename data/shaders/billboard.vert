#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

out vec2 vary_uv;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    vary_uv = uv;
    gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(pos, 1.0);
}
