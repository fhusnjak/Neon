#pragma once

#include "Renderer/RenderPass.h"
#include "Vulkan.h"

namespace Neon
{
	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(RenderPassSpecification specification);
		~VulkanRenderPass() = default;

		RenderPassSpecification& GetSpecification() override
		{
			return m_Specification;
		}
		const RenderPassSpecification& GetSpecification() const override
		{
			return m_Specification;
		}

		vk::RenderPass GetHandle() const
		{
			return m_Handle.get();
		}

	private:
		RenderPassSpecification m_Specification;

		vk::UniqueRenderPass m_Handle;
	};
} // namespace Neon
