#pragma once

namespace Neon
{
	struct RenderPassSpecification
	{
	};

	class RenderPass : public RefCounted
	{
	public:
		virtual ~RenderPass() = default;

		virtual RenderPassSpecification& GetSpecification() = 0;
		virtual const RenderPassSpecification& GetSpecification() const = 0;

		static SharedRef<RenderPass> Create(const RenderPassSpecification& spec);
	};
} // namespace Neon
