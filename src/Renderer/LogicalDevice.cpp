//
// Created by Filip on 4.8.2020..
//

#include "LogicalDevice.h"
#include "Context.h"
LogicalDevice::LogicalDevice(const PhysicalDevice& physicalDevice)
{
	std::set<uint32_t> queueFamilyIndices = {physicalDevice.GetGraphicsQueueFamily().m_Index,
											 physicalDevice.GetComputeQueueFamily().m_Index};
	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(queueFamilyIndices.size());
	for (uint32_t queueFamilyIndex : queueFamilyIndices)
	{
		queueCreateInfos.push_back({{}, queueFamilyIndex, 1, &queuePriority});
	}
	vk::PhysicalDeviceScalarBlockLayoutFeatures scalarLayoutFeatures;
	scalarLayoutFeatures.scalarBlockLayout = VK_TRUE;
	vk::PhysicalDeviceDescriptorIndexingFeatures descriptorFeatures;
	descriptorFeatures.runtimeDescriptorArray = VK_TRUE;
	descriptorFeatures.pNext = &scalarLayoutFeatures;
	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	vk::PhysicalDeviceFeatures2 deviceFeatures2;
	deviceFeatures2.pNext = &descriptorFeatures;
	deviceFeatures2.features = deviceFeatures;
	std::vector<vk::PhysicalDeviceFeatures> features = {deviceFeatures};
#ifdef NDEBUG
	vk::DeviceCreateInfo deviceCreateInfo{
		{},
		static_cast<uint32_t>(queueCreateInfos.size()),
		queueCreateInfos.data(),
		0,
		nullptr,
		static_cast<uint32_t>(physicalDevice.GetRequiredExtensions().size()),
		physicalDevice.GetRequiredExtensions().data(),
		nullptr};
#else
	vk::DeviceCreateInfo deviceCreateInfo{
		{},
		static_cast<uint32_t>(queueCreateInfos.size()),
		queueCreateInfos.data(),
		static_cast<uint32_t>(Context::GetInstance().GetValidationLayers().size()),
		Context::GetInstance().GetValidationLayers().data(),
		static_cast<uint32_t>(physicalDevice.GetRequiredExtensions().size()),
		physicalDevice.GetRequiredExtensions().data(),
		nullptr};
#endif
	deviceCreateInfo.pNext = &deviceFeatures2;
	m_Handle = physicalDevice.GetHandle().createDevice(deviceCreateInfo);

	m_GraphicsQueue = m_Handle.getQueue(physicalDevice.GetGraphicsQueueFamily().m_Index, 0);
	m_PresentQueue = m_Handle.getQueue(physicalDevice.GetComputeQueueFamily().m_Index, 0);
}
