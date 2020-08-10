//
// Created by Filip on 10.8.2020..
//

#include "SwapChain.h"

std::unique_ptr<Neon::SwapChain> Neon::SwapChain::Create(const Neon::PhysicalDevice& physicalDevice,
														 const Neon::LogicalDevice& logicalDevice,
														 vk::SurfaceKHR surface,
														 vk::Extent2D extent)
{
	auto swapChain = new Neon::SwapChain(physicalDevice, logicalDevice, surface, extent);
	return std::unique_ptr<Neon::SwapChain>(swapChain);
}

Neon::SwapChain::SwapChain(const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice,
						   vk::SurfaceKHR surface, vk::Extent2D extent)
{
	auto deviceSurfaceProperties = physicalDevice.GetDeviceSurfaceProperties(surface);
	std::vector<vk::SurfaceFormatKHR> availableSurfaceFormats = deviceSurfaceProperties.formats;
	assert(!availableSurfaceFormats.empty());
	vk::SurfaceFormatKHR surfaceFormat = availableSurfaceFormats[0];
	for (const auto& availableSurfaceFormat : availableSurfaceFormats)
	{
		if (availableSurfaceFormat.format == vk::Format::eB8G8R8A8Unorm &&
			availableSurfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{ surfaceFormat = availableSurfaceFormat; }
	}
	m_SwapChainImageFormat = surfaceFormat.format;

	std::vector<vk::PresentModeKHR> availablePresentModes = deviceSurfaceProperties.presentModes;
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{ presentMode = availablePresentMode; }
	}

	uint32_t imageCount = deviceSurfaceProperties.surfaceCapabilities.minImageCount + 1;
	if (deviceSurfaceProperties.surfaceCapabilities.maxImageCount > 0 &&
		imageCount > deviceSurfaceProperties.surfaceCapabilities.maxImageCount)
	{ imageCount = deviceSurfaceProperties.surfaceCapabilities.maxImageCount; }
	vk::SwapchainCreateInfoKHR swapChainCreateInfo(
		{}, surface, imageCount, m_SwapChainImageFormat, surfaceFormat.colorSpace, extent, 1,
		vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr,
		deviceSurfaceProperties.surfaceCapabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, nullptr);

	if (physicalDevice.GetPresentQueueFamily().m_Index !=
		physicalDevice.GetGraphicsQueueFamily().m_Index)
	{
		uint32_t queueFamilyIndices[] = {physicalDevice.GetGraphicsQueueFamily().m_Index,
										 physicalDevice.GetPresentQueueFamily().m_Index};
		swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	m_Handle.reset();
	m_Handle = logicalDevice.GetHandle().createSwapchainKHRUnique(swapChainCreateInfo);
	auto swapChainImages = logicalDevice.GetHandle().getSwapchainImagesKHR(m_Handle.get());
	m_SwapChainImageViews.reserve(swapChainImages.size());
	for (auto& swapChainImage : swapChainImages)
	{
		vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		vk::ImageViewCreateInfo imageViewCreateInfo{
			{}, swapChainImage,	 vk::ImageViewType::e2D, m_SwapChainImageFormat,
			{}, subResourceRange};
		m_SwapChainImageViews.push_back(
			logicalDevice.GetHandle().createImageViewUnique(imageViewCreateInfo));
	}
}
