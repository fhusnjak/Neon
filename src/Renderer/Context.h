//
// Created by Filip on 4.8.2020..
//

#ifndef NEON_CONTEXT_H
#define NEON_CONTEXT_H

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include <utility>
#include <vulkan/vulkan.hpp>

namespace Neon
{
class Context
{
public:
	void Init();
	static Context& GetInstance()
	{
		return s_Instance;
	}
	[[nodiscard]] const vk::Instance& GetVkInstance() const
	{
		return m_VkInstance.get();
	}
	[[nodiscard]] const std::vector<const char*>& GetValidationLayers() const
	{
		return m_ValidationLayers;
	}
	[[nodiscard]] const std::vector<const char*>& GetDeviceExtensions() const
	{
		return m_DeviceExtensions;
	}
	void CreateDevice(const vk::SurfaceKHR& surface,
					  const std::vector<vk::QueueFlagBits>& queueFlags);
	PhysicalDevice& GetPhysicalDevice();
	LogicalDevice& GetLogicalDevice();
	void InitAllocator();

private:
	Context() noexcept;
	bool CheckValidationLayerSupport();
	bool CheckExtensionSupport();
	std::vector<const char*> GetRequiredExtensions()
	{
		return m_InstanceExtensions;
	}

private:
	static Context s_Instance;
	vk::UniqueInstance m_VkInstance;
	std::unique_ptr<PhysicalDevice> m_PhysicalDevice;
	std::shared_ptr<LogicalDevice> m_LogicalDevice;
	const std::vector<const char*> m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
	const std::vector<const char*> m_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
	std::vector<const char*> m_InstanceExtensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
};
} // namespace Neon

#endif //NEON_CONTEXT_H
