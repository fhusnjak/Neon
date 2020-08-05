#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "material.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in int matID;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) flat out int fragMatID;

layout(binding = 0, scalar) uniform CameraMatrices
{
    vec3 cameraPos;
    mat4 view;
    mat4 projection;
} cameraMatrices;

layout(push_constant, scalar) uniform PushConstant
{
    mat4 model;
    vec3 lightPosition;
    vec3 lightColor;
}
pushConstant;

void main()
{
    fragColor = color;
    fragNorm = normalize(norm);
    fragPos = pos;
    fragTexCoord = texCoord;
    fragMatID = matID;

    gl_Position = cameraMatrices.projection * cameraMatrices.view * pushConstant.model * vec4(pos, 1.0);
}