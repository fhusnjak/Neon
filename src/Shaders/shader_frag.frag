#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "material.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) flat in int fragMatID;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) uniform CameraMatrices
{
    vec3 cameraPos;
    mat4 view;
    mat4 projection;
} cameraMatrices;
layout(set = 1, binding = 0, scalar) readonly buffer MaterialBufferObject
{
    Material materials[];
};
layout(set = 1, binding = 1) uniform sampler2D textureSamplers[];

layout(push_constant, scalar) uniform PushConstant
{
    mat4 model;
    vec3 lightPosition;
    vec3 lightColor;
}
pushConstant;

void main()
{
    vec3 normal = normalize(fragNorm);

    vec3 worldPos = vec3(pushConstant.model * vec4(fragPos, 1.0));

    Material mat = materials[fragMatID];

    vec3 lightDir = pushConstant.lightPosition - worldPos;
    float lightIntensity = 50.0f;
    float d = length(lightDir);
    lightIntensity = 50.0f / (d * d);
    lightDir = normalize(lightDir);

    vec3 diffuse = computeDiffuse(mat, lightDir, normal);
    if (mat.textureId >= 0)
    {
        vec3 diffuseTxt = texture(textureSamplers[mat.textureId], fragTexCoord).xyz;
        diffuse *= diffuseTxt;
    }

    vec3 viewDir = normalize(cameraMatrices.cameraPos - worldPos);
    vec3 specular = computeSpecular(mat, viewDir, lightDir, normal);

    float gamma = 1. / 2.2;
    outColor = pow(vec4(lightIntensity * (diffuse + specular), 1.0f), vec4(gamma));
}