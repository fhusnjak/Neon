#pragma once

#include <vulkan/vulkan.hpp>

namespace Neon
{
class DescriptorSet
{
public:
	void Init(vk::Device device);
	void CreateDescriptorSet(vk::DescriptorPool pool,
							 const std::vector<vk::DescriptorSetLayoutBinding>& bindings);

	vk::WriteDescriptorSet CreateWrite(size_t binding, const vk::DescriptorBufferInfo* info,
									   uint32_t arrayElement);

	vk::WriteDescriptorSet CreateWrite(size_t binding, const vk::DescriptorImageInfo* info,
									   uint32_t arrayElement);

	void Update(std::vector<vk::WriteDescriptorSet>& descriptorWrites);

	[[nodiscard]] const vk::DescriptorSet& Get() const
	{
		return m_Set;
	}

	[[nodiscard]] const vk::DescriptorSetLayout& GetLayout() const
	{
		return m_Layout;
	}

private:
	vk::Device m_Device;
	std::vector<vk::DescriptorSetLayoutBinding> m_Bindings;
	vk::DescriptorSetLayout m_Layout;
	vk::DescriptorSet m_Set;
};
} // namespace Neon
