#include "neopch.h"

#include "VulkanTools.h"

vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
							   const vk::FormatFeatureFlags& features,
							   vk::PhysicalDevice& physicalDevice)
{
	for (auto& format : candidates)
	{
		vk::FormatProperties props = physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear &&
				(props.linearTilingFeatures & features) == features ||
			tiling == vk::ImageTiling::eOptimal &&
				(props.optimalTilingFeatures & features) == features)
		{ return format; }
	}

	throw std::runtime_error("Format not found");
}

vk::Format FindDepthFormat(vk::PhysicalDevice& physicalDevice)
{
	return FindSupportedFormat(
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment,
		physicalDevice);
}
