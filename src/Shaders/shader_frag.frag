#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "material.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragCameraPos;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) flat in int fragMatID;
layout(location = 6) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1, scalar) readonly buffer MaterialBufferObject { Material m[]; } materials[];
layout(binding = 2) uniform sampler2D textureSamplers[];
layout(binding = 3, scalar) readonly buffer InstanceBufferObject { Instance i[]; } instances;

layout(push_constant, scalar) uniform PushConstant
{
    int instanceID;
    vec3 lightPosition;
    vec3 lightColor;
}
pushC;

void main()
{
    int objId = instances.i[pushC.instanceID].objId;
    Material mat = materials[objId].m[fragMatID];

    vec3 lightDir = pushC.lightPosition - worldPos;
    float lightIntensity = 50.0f;
    float d = length(lightDir);
    lightIntensity = 50.0f / (d * d);
    lightDir = normalize(lightDir);

    vec3 diffuse = computeDiffuse(mat, lightDir, fragNorm);
    if (mat.textureId >= 0)
    {
        int txtOffset = instances.i[pushC.instanceID].texOffset;
        uint txtId = txtOffset + mat.textureId;
        vec3 diffuseTxt = texture(textureSamplers[txtId], fragTexCoord).xyz;
        diffuse *= diffuseTxt;
    }

    vec3 viewDir = normalize(fragCameraPos - worldPos);
    vec3 specular = computeSpecular(mat, viewDir, lightDir, fragNorm);

    float gamma = 1. / 2.2;
    outColor = pow(vec4(lightIntensity * (diffuse + specular), 1.0f), vec4(gamma));
}