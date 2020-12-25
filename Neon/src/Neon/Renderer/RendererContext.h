#pragma once

namespace Neon
{
	class RendererContext
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void BeginFrame() = 0;
		virtual void SwapBuffers() = 0;

		virtual void OnResize(uint32 width, uint32 height) = 0;

		static UniquePtr<RendererContext> Create(void* window);
	};
} // namespace Neon
