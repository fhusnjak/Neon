#include "neopch.h"

#include "VertexBuffer.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanVertexBuffer.h"

namespace Neon
{
	SharedRef<VertexBuffer> VertexBuffer::Create(void* data, uint32 size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:
				NEO_CORE_ASSERT(false, "Renderer API not selected!");
				return nullptr;
			case RendererAPI::API::Vulkan:
				return nullptr;
				//return SharedRef<VulkanVertexBuffer>::Create(data, size);
		}
		NEO_CORE_ASSERT(false, "Renderer API not selected!");
		return nullptr;
	}
} // namespace Neon
