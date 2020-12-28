#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    float color;
} ubo[2];

void main()
{
    outColor = vec4(ubo[0].color, ubo[1].color, 0.f, 1.f);
}
