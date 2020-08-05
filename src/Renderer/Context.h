//
// Created by Filip on 4.8.2020..
//

#ifndef NEON_CONTEXT_H
#define NEON_CONTEXT_H

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include <vulkan/vulkan.hpp>

enum class DeviceType
{
	GRAPHICS_DEVICE,
	COMPUTING_DEVICE
};

struct DeviceBundle
{
	PhysicalDevice m_PhysicalDevice;
	LogicalDevice m_LogicalDevice;
	DeviceBundle(const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice)
		: m_PhysicalDevice(physicalDevice)
		, m_LogicalDevice(logicalDevice){};
};

class Context
{

public:
	void Init();
	static Context& GetInstance()
	{
		return s_Instance;
	}
	const vk::Instance& GetVkInstance()
	{
		return m_VkInstance.get();
	}
	std::vector<const char*>& GetEnabledLayers()
	{
		return m_EnabledLayers;
	}
	[[nodiscard]] const std::vector<const char*>& GetValidationLayers() const
	{
		return m_ValidationLayers;
	}
	[[nodiscard]] const std::vector<const char*>& GetDeviceExtensions() const
	{
		return m_DeviceExtensions;
	}
	void AddDevice(const DeviceType& deviceType, const PhysicalDevice& physicalDevice,
				   const LogicalDevice& logicalDevice);
	DeviceBundle GetDeviceBundle(DeviceType deviceType);

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
	std::unordered_map<DeviceType, DeviceBundle> m_Devices;
	std::vector<const char*> m_EnabledLayers;
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

#endif //NEON_CONTEXT_H
