#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 vertex_uv;
layout (location = 2) in vec3 vertex_normal;

noperspective out vec2 uv;
out vec3 LIGHT;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform int internalResolutionWidth;
uniform int internalResolutionHeight;

void main()
{
    uv = vertex_uv;

    vec4 vertInClipSpace = projMatrix * viewMatrix * modelMatrix * vec4(vertex_pos, 1.0);

    /* Polygon Jittering */
    vec2 grid = 0.5 * vec2(internalResolutionWidth, internalResolutionHeight);
    vec3 vertInNDC = vertInClipSpace.xyz / vertInClipSpace.w;
    vertInNDC.xy = floor(vertInNDC.xy * grid) / grid;
    vertInClipSpace.xyz = vertInNDC * vertInClipSpace.w;

    gl_Position = vertInClipSpace;

    const vec4 sunlightWCS = vec4(-0.5, -1.0, -1.0, 0.0);
    vec3 normalWCS = transpose(inverse(mat3(modelMatrix))) * vertex_normal;

    float ambientIntensity = 0.3;
    float diffuseIntensity = max(0.0, dot(normalize(normalWCS), normalize(-vec3(sunlightWCS))));
    vec3 lightAMB = vec3(1.0) * ambientIntensity;
    vec3 lightDIFF = vec3(1.0) * diffuseIntensity;
    LIGHT = lightAMB + lightDIFF;
}
