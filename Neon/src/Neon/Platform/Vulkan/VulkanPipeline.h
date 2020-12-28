#pragma once

#include "Renderer/Pipeline.h"
#include "Vulkan.h"

namespace Neon
{
	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecification& specification);
		~VulkanPipeline() = default;

		vk::PipelineLayout GetLayout() const
		{
			return m_PipelineLayout.get();
		}

		PipelineSpecification& GetSpecification() override
		{
			return m_Specification;
		}
		const PipelineSpecification& GetSpecification() const override
		{
			return m_Specification;
		}

		vk::Pipeline GetHandle() const
		{
			return m_Handle.get();
		}

	private:
		PipelineSpecification m_Specification;

		vk::UniquePipelineLayout m_PipelineLayout;
		vk::UniquePipeline m_Handle;
	};
} // namespace Neon
