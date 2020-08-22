#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "material.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec4 fragClipSpace;
layout(location = 3) in vec2 fragTextureCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) readonly buffer MaterialBufferObject
{
    Material material;
};
layout(set = 0, binding = 1) uniform sampler2D textureSamplerRefraction;
layout(set = 0, binding = 2) uniform sampler2D textureSamplerReflection;
layout(set = 0, binding = 3) uniform sampler2D dudvMap;

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

    float moveFactor;
}
pushConstant;

float waveStrength = 0.01;

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

    vec2 distortion1 = (texture(dudvMap, vec2(fragTextureCoords.x + pushConstant.moveFactor, fragTextureCoords.y)).rg * 2 - 1) * waveStrength;
    vec2 distortion2 = (texture(dudvMap, vec2(-fragTextureCoords.x + pushConstant.moveFactor, fragTextureCoords.y + pushConstant.moveFactor)).rg * 2 - 1) * waveStrength;
    vec2 distortion = distortion1 + distortion2;

    vec2 textureCoords = fragClipSpace.xy / fragClipSpace.w;
    textureCoords = textureCoords / 2 + 0.5;

    vec2 refractTextureCoords = vec2(textureCoords.x, textureCoords.y);
    vec2 reflectTextureCoords = vec2(textureCoords.x, -textureCoords.y);

    refractTextureCoords += distortion;
    refractTextureCoords = clamp(refractTextureCoords, 0.001, 0.999);

    reflectTextureCoords += distortion;
    reflectTextureCoords.x = clamp(reflectTextureCoords.x, 0.001, 0.999);
    reflectTextureCoords.y = clamp(reflectTextureCoords.y, -0.999, -0.001);

    vec4 textureValue = 0.5 * texture(textureSamplerRefraction, refractTextureCoords) + 0.5 * texture(textureSamplerReflection, reflectTextureCoords);

    vec3 diffuse = computeDiffuse(material, lightDir, fragNorm) * textureValue.rgb;

    vec3 viewDir = normalize(pushConstant.cameraPos - fragWorldPos);
    vec3 specular = computeSpecular(material, viewDir, lightDir, fragNorm);

    float gamma = 1. / 2.2;
    outColor = pow(vec4(lightIntensity * (diffuse + specular), 1.0), vec4(gamma));
}