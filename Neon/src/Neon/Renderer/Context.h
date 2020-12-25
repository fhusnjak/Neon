//
// Created by Filip on 4.8.2020..
//

#ifndef NEON_CONTEXT_H
#define NEON_CONTEXT_H

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include <Window/Window.h>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace Neon
{
class Context
{
public:

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
	Window* m_Window = nullptr;
	std::unique_ptr<PhysicalDevice> m_PhysicalDevice;
	std::shared_ptr<LogicalDevice> m_LogicalDevice;
	vk::UniqueSurfaceKHR m_Surface;
	const std::vector<const char*> m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
	const std::vector<const char*> m_DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
														 VK_KHR_MAINTENANCE2_EXTENSION_NAME,
														 VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
														 VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
														 VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
														 VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
														 VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
														 VK_KHR_MULTIVIEW_EXTENSION_NAME};
	std::vector<const char*> m_InstanceExtensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
};
} // namespace Neon

#endif //NEON_CONTEXT_H
