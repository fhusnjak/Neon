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
layout(set = 0, binding = 4) uniform sampler2D normalMap;
layout(set = 0, binding = 5) uniform sampler2D depthMap;

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

float waveStrength = 0.15;

void main()
{
    vec3 lightDir = normalize(-pushConstant.lightDirection);
    float lightIntensity = pushConstant.lightIntensity;
    if (pushConstant.pointLight == 1)
    {
        lightDir = pushConstant.lightPosition - fragWorldPos;
        lightIntensity /= length(lightDir);
        lightDir = normalize(lightDir);
    }

    vec3 viewDir = pushConstant.cameraPos - fragWorldPos;
    float distanceFromCamera = length(viewDir);
    viewDir = normalize(viewDir);

    float near = 0.1;
    float far = 15000.0;
    vec2 textureCoords = fragClipSpace.xy / fragClipSpace.w;
    textureCoords = textureCoords / 2 + 0.5;
    vec2 refractTextureCoords = vec2(textureCoords.x, textureCoords.y);
    vec2 reflectTextureCoords = vec2(textureCoords.x, -textureCoords.y);
    float depth = texture(depthMap, refractTextureCoords).r;
    float floorDist = 2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));
    float waterDepth = floorDist - distanceFromCamera;

    vec2 distortedTexCoords = texture(dudvMap, vec2(fragTextureCoords.x + pushConstant.moveFactor, fragTextureCoords.y)).rg * 0.1;
    distortedTexCoords = fragTextureCoords + vec2(distortedTexCoords.x, distortedTexCoords.y + pushConstant.moveFactor);
    vec2 distortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength / distanceFromCamera;

    refractTextureCoords += distortion;
    refractTextureCoords = clamp(refractTextureCoords, 0.001, 0.999);

    reflectTextureCoords += distortion;
    reflectTextureCoords.x = clamp(reflectTextureCoords.x, 0.001, 0.999);
    reflectTextureCoords.y = clamp(reflectTextureCoords.y, -0.999, -0.001);

    float F0 = 0.1;
    float reflectionFactor = F0 + (1 - F0) * pow(1 - abs(dot(viewDir, vec3(0, 1, 0))), 5);
    vec4 textureValue = mix(texture(textureSamplerRefraction, refractTextureCoords), texture(textureSamplerReflection, reflectTextureCoords), reflectionFactor);

    vec4 normalMapColor = texture(normalMap, distortedTexCoords);
    vec3 normal = normalize(vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b, normalMapColor.g * 2.0 - 1.0));

    vec3 specular = computeSpecular(material, viewDir, lightDir, normal);

    outColor = (mix(textureValue, vec4(0.0, 0.3, 0.5, 1.0), 0.2) + lightIntensity * vec4(specular, 0.0));
    //outColor.a = clamp(waterDepth, 0.0, 1.0);
    //outColor = vec4(depth, depth, depth, 1.0);
}