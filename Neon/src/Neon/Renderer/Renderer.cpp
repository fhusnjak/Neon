#include "neopch.h"

#include "Renderer.h"

namespace Neon
{
	UniquePtr<RendererAPI> Renderer::s_RendererAPI = RendererAPI::Create();
}
