#include "neopch.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Renderer.h"
#include "RendererContext.h"

namespace Neon
{
	UniquePtr<RendererContext> RendererContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				NEO_CORE_ASSERT(false, "RendererAPI is not selected");
				return nullptr;
			case RendererAPI::API::Vulkan:
				return CreateUnique<VulkanContext>(static_cast<GLFWwindow*>(window));
		}
		NEO_CORE_ASSERT(false, "Unknown RendererAPI is selected");
		return nullptr;
	}
} // namespace Neon
