#include "neopch.h"

#include "VulkanAllocator.h"
#include "VulkanContext.h"

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

	void VulkanAllocator::Allocate(vk::MemoryRequirements requirements, vk::UniqueDeviceMemory& outDeviceMemory,
								   vk::MemoryPropertyFlags flags /*= vk::MemoryPropertyFlagBits::eDeviceLocal*/)
	{
		NEO_CORE_ASSERT(m_Device, "Allocator is not initialized with existing device");

		NEO_CORE_TRACE("VulkanAllocator ({0}): allocating {1} bytes", m_Tag, requirements.size);

		vk::MemoryAllocateInfo memAlloc = {};
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		outDeviceMemory = m_Device->GetHandle().allocateMemoryUnique(memAlloc);
	}

	void VulkanAllocator::AllocateUniformBuffer(UniformBuffer& outUniformBuffer, uint32 size)
	{
		vk::Device device = VulkanContext::GetDevice()->GetHandle();

		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.size = size;
		bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		outUniformBuffer.Buffer = device.createBufferUnique(bufferInfo);

		vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(outUniformBuffer.Buffer.get());
		Allocate(memRequirements, outUniformBuffer.Memory,
				 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		device.bindBufferMemory(outUniformBuffer.Buffer.get(), outUniformBuffer.Memory.get(), 0);
		outUniformBuffer.Size = size;
	}

	void VulkanAllocator::UpdateUniformBuffer(UniformBuffer& outUniformBuffer, const void* data)
	{
		vk::Device device = VulkanContext::GetDevice()->GetHandle();

		void* dest;
		device.mapMemory(outUniformBuffer.Memory.get(), 0, outUniformBuffer.Size, vk::MemoryMapFlags(), &dest);
		memcpy(dest, data, outUniformBuffer.Size);
		device.unmapMemory(outUniformBuffer.Memory.get());
	}

} // namespace Neon
