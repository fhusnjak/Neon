#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 worldPosition;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, scalar) uniform CameraMatrices
{
    vec3 cameraPos;
    mat4 view;
    mat4 projection;
} cameraMatrices;

const vec3 baseColor = vec3(0.18,0.27,0.47);

void main()
{
    float red = -0.00022*(abs(worldPosition.y)-2800) + 0.18;
    float green = -0.00025*(abs(worldPosition.y)-2800) + 0.27;
    float blue = -0.00019*(abs(worldPosition.y)-2800) + 0.5;
    outColor = vec4(red, green, blue, 1);
}