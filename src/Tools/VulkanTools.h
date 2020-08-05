#pragma once

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
							   const vk::FormatFeatureFlags& features,
							   vk::PhysicalDevice& physicalDevice);

vk::Format FindDepthFormat(vk::PhysicalDevice& physicalDevice);
