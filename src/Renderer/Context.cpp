//
// Created by Filip on 4.8.2020..
//

#include "Context.h"
#include <Core/Allocator.h>
#include <GLFW/glfw3.h>

Context Context::s_Instance;

Context::Context() noexcept { }

void Context::Init()
{
	if (!glfwVulkanSupported())
	{
		std::cout << "ERROR: Vulkan not supported" << std::endl;
		exit(1);
	}

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	m_InstanceExtensions.insert(m_InstanceExtensions.end(), glfwExtensions,
								glfwExtensions + glfwExtensionCount);
	assert(CheckExtensionSupport());
	vk::ApplicationInfo applicationInfo("Neon", 1, "Vulkan engine", 1, VK_API_VERSION_1_0);
#ifdef NDEBUG
	vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, 0, nullptr,
											  static_cast<uint32_t>(m_InstanceExtensions.size()),
											  m_InstanceExtensions.data());
#else
	assert(CheckValidationLayerSupport());
	vk::InstanceCreateInfo instanceCreateInfo(
		{}, &applicationInfo, static_cast<uint32_t>(m_ValidationLayers.size()),
		m_ValidationLayers.data(), static_cast<uint32_t>(m_InstanceExtensions.size()),
		m_InstanceExtensions.data());
#endif
	m_VkInstance = vk::createInstanceUnique(instanceCreateInfo);
}

void Context::CreateDevice(const vk::SurfaceKHR& surface,
						   const std::vector<vk::QueueFlagBits>& queueFlags)
{
	assert(m_PhysicalDevice == nullptr);
	assert(m_LogicalDevice == nullptr);
	m_PhysicalDevice = PhysicalDevice::Create(surface, m_DeviceExtensions, queueFlags);
	m_LogicalDevice = LogicalDevice::Create(*m_PhysicalDevice);
}

bool Context::CheckExtensionSupport()
{
	std::vector<vk::ExtensionProperties> supportedExtensions =
		vk::enumerateInstanceExtensionProperties();
	for (const auto& extension : m_InstanceExtensions)
	{
		bool found = false;
		for (const auto& supportedExtension : supportedExtensions)
		{
			if (strcmp(supportedExtension.extensionName, extension) != 0)
			{
				found = true;
				break;
			}
		}
		if (!found) { return false; }
	}
	return true;
}

bool Context::CheckValidationLayerSupport()
{
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
	for (auto layerName : m_ValidationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound) { return false; }
	}
	return true;
}

PhysicalDevice& Context::GetPhysicalDevice()
{
	assert(m_PhysicalDevice != nullptr);
	return *m_PhysicalDevice;
}

LogicalDevice& Context::GetLogicalDevice()
{
	assert(m_LogicalDevice != nullptr);
	return *m_LogicalDevice;
}
void Context::InitAllocator()
{
	Allocator::Init(m_PhysicalDevice->GetHandle(), m_LogicalDevice->GetHandle());
}
