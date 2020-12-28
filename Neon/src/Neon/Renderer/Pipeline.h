#pragma once

#include "RenderPass.h"
#include "Shader.h"

namespace Neon
{
	struct PipelineSpecification
	{
		SharedRef<Shader> Shader;
		SharedRef<RenderPass> RenderPass;
	};

	class Pipeline : public RefCounted
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		static SharedRef<Pipeline> Create(const PipelineSpecification& spec);
	};
} // namespace Neon