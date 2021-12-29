#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 vertex_uv;
layout (location = 2) in vec3 vertex_normal;

out vec2 uv;
out vec3 LIGHT;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform bool dontShade;

void main()
{
    uv = vertex_uv;

    gl_Position =  projMatrix * viewMatrix * modelMatrix * vec4(vertex_pos, 1.0);

    const vec4 sunlightWCS = vec4(-0.5, -1.0, -1.0, 0.0);
    vec3 normalWCS = transpose(inverse(mat3(modelMatrix))) * vertex_normal;

    if(dontShade)
    {
        LIGHT = vec3(1.0);
    }
    else
    {
        float ambientIntensity = 0.3;
        float diffuseIntensity = max(0.0, dot(normalize(normalWCS), normalize(-vec3(sunlightWCS))));
        vec3 lightAMB = vec3(1.0) * ambientIntensity;
        vec3 lightDIFF = vec3(1.0) * diffuseIntensity;
        LIGHT = lightAMB + lightDIFF;
    }

}
