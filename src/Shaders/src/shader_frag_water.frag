#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "material.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec4 fragClipSpace;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) readonly buffer MaterialBufferObject
{
    Material material;
};
layout(set = 0, binding = 1) uniform sampler2D textureSamplerRefraction;
layout(set = 0, binding = 2) uniform sampler2D textureSamplerReflection;

layout(push_constant, scalar) uniform PushConstant
{
    vec3 cameraPos;
    mat4 view;
    mat4 projection;

    vec4 clippingPlane;

    mat4 model;

    int pointLight;
    float lightIntensity;
    vec3 lightDirection;
    vec3 lightPosition;
}
pushConstant;

void main()
{
    vec3 lightDir = normalize(-pushConstant.lightDirection);
    float lightIntensity = pushConstant.lightIntensity;
    if (pushConstant.pointLight == 1)
    {
        lightDir = pushConstant.lightPosition - fragWorldPos;
        float d = length(lightDir);
        lightIntensity = pushConstant.lightIntensity / d;
    }

    vec2 textureCoords = fragClipSpace.xy / fragClipSpace.w;
    textureCoords = textureCoords / 2 + 0.5;

    vec4 textureValue = 0.5 * texture(textureSamplerRefraction, vec2(textureCoords.x, textureCoords.y)) + 0.5 * texture(textureSamplerReflection, vec2(textureCoords.x, -textureCoords.y));

    vec3 diffuse = computeDiffuse(material, lightDir, fragNorm) * textureValue.rgb;

    vec3 viewDir = normalize(pushConstant.cameraPos - fragWorldPos);
    vec3 specular = computeSpecular(material, viewDir, lightDir, fragNorm);

    float gamma = 1. / 2.2;
    outColor = pow(vec4(lightIntensity * (diffuse + specular), 1.0), vec4(gamma));
}