#pragma once

#include "Vulkan.h"
#include "VulkanDevice.h"

#include <string>

namespace Neon
{
	struct UniformBuffer
	{
		vk::UniqueDeviceMemory Memory;
		vk::UniqueBuffer Buffer;
		uint32 Size;
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		VulkanAllocator(const std::string& tag);
		VulkanAllocator(const SharedRef<VulkanDevice>& device, const std::string& tag = "");
		~VulkanAllocator() = default;

		void Allocate(vk::MemoryRequirements requirements, vk::UniqueDeviceMemory& outDeviceMemory,
					  vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits::eDeviceLocal);

		void AllocateUniformBuffer(UniformBuffer& outUniformBuffer, uint32 size);

		void UpdateUniformBuffer(UniformBuffer& outUniformBuffer, const void* data);

	private:
		SharedRef<VulkanDevice> m_Device;
		std::string m_Tag;
	};
} // namespace Neon
