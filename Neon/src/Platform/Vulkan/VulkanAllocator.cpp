#include "neopch.h"

#include "VulkanAllocator.h"

namespace Neon
{
	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_Tag(tag)
	{
	}

	VulkanAllocator::VulkanAllocator(const SharedRef<VulkanDevice>& device, const std::string& tag /*= ""*/)
		: m_Device(device)
		, m_Tag(tag)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

	void VulkanAllocator::Allocate(vk::MemoryRequirements requirements, vk::UniqueDeviceMemory& dest,
								   vk::MemoryPropertyFlags flags /*= vk::MemoryPropertyFlagBits::eDeviceLocal*/)
	{
		NEO_CORE_ASSERT(m_Device, "Allocator is not initialized with existing device");

		NEO_CORE_TRACE("VulkanAllocator ({0}): allocating {1} bytes", m_Tag, requirements.size);

		vk::MemoryAllocateInfo memAlloc = {};
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		dest = m_Device->GetHandle().allocateMemoryUnique(memAlloc);		
	}

}

