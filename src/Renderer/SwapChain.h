//
// Created by Filip on 10.8.2020..
//

#ifndef NEON_SWAPCHAIN_H
#define NEON_SWAPCHAIN_H

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "vulkan/vulkan.hpp"

namespace Neon
{
class SwapChain
{
public:
	SwapChain(const SwapChain&) = delete;
	SwapChain(SwapChain&&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;
	SwapChain& operator=(SwapChain&&) = delete;
	static std::unique_ptr<SwapChain> Create(const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice, vk::SurfaceKHR surface, vk::Extent2D extent);
	[[nodiscard]] const vk::SwapchainKHR& GetHandle() const
	{
		return m_Handle.get();
	}
	[[nodiscard]] const std::vector<vk::UniqueImageView>& GetImageViews() const
	{
		return m_SwapChainImageViews;
	}
	[[nodiscard]] vk::Format GetSwapChainImageFormat() const
	{
		return m_SwapChainImageFormat;
	}
private:
	SwapChain(const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice, vk::SurfaceKHR surface, vk::Extent2D extent);
private:
	vk::UniqueSwapchainKHR m_Handle;
	vk::Format m_SwapChainImageFormat;
	std::vector<vk::UniqueImageView> m_SwapChainImageViews;
};
}

#endif //NEON_SWAPCHAIN_H
