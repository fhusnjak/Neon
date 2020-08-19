#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) uniform CameraMatrices
{
    vec3 cameraPos;
    mat4 view;
    mat4 projection;
} cameraMatrices;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(push_constant, scalar) uniform PushConstant
{
    mat4 model;
    float lightIntensity;
    vec3 lightPosition;
    vec3 lightColor;
}
pushConstant;

void main()
{
    outColor = texture(textureSampler, fragTexCoord);
}