#pragma once

#include "Neon/Renderer/RendererAPI.h"
#include "Vulkan.h"

namespace Neon
{
	class VulkanRendererAPI : public RendererAPI
	{
	public:
		~VulkanRendererAPI() override;

		void Init() override;
		void Render() override;

		void Shutdown() override;
	};
} // namespace Neon
