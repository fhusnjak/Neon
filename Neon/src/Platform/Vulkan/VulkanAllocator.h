#pragma once

#include "Vulkan.h"
#include "VulkanDevice.h"

#include <string>

namespace Neon
{
	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		VulkanAllocator(const std::string& tag);
		VulkanAllocator(const SharedRef<VulkanDevice>& device, const std::string& tag = "");
		~VulkanAllocator();

		void Allocate(vk::MemoryRequirements requirements, vk::UniqueDeviceMemory& dest,
					  vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits::eDeviceLocal);

	private:
		SharedRef<VulkanDevice> m_Device;
		std::string m_Tag;
	};
} // namespace Neon
