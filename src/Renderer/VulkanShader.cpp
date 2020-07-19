#include "vkpch.h"

#include "VulkanShader.h"

#include "Tools/FileTools.h"

VulkanShader::VulkanShader(vk::Device device, vk::ShaderStageFlagBits stage)
	: m_Device(device)
	, m_Stage(stage)
{
}

void VulkanShader::LoadFromFile(const std::string& fileName)
{
	std::vector<char> code = ReadFile(fileName);
	vk::ShaderModuleCreateInfo createInfo{
		{}, code.size(), reinterpret_cast<const uint32_t*>(code.data())};
	m_Module = m_Device.createShaderModuleUnique(createInfo);
}

vk::PipelineShaderStageCreateInfo VulkanShader::GetShaderStageCreateInfo() const
{
	return {{}, m_Stage, m_Module.get(), "main"};
}
