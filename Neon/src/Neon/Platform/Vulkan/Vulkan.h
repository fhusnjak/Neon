#pragma once

#include <vulkan/vulkan.hpp>

#define VK_CHECK_RESULT(f)                                                                                                         \
	{                                                                                                                              \
		vk::Result res = (f);                                                                                                      \
		NEO_CORE_ASSERT(res == vk::Result::eSuccess);                                                                                       \
	}\
