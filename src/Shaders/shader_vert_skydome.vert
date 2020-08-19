#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 fragPos;

layout(set = 0, binding = 0, scalar) uniform CameraMatrices
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
    fragPos = (pushConstant.model * vec4(pos, 1.0)).xyz;
    gl_Position = cameraMatrices.projection * cameraMatrices.view * pushConstant.model * vec4(pos, 1.0);
}