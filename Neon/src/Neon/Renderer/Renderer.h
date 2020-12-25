#pragma once

#include "Neon/Core/Application.h"
#include "RendererAPI.h"
#include "RendererContext.h"

namespace Neon
{
	class Renderer
	{
	public:
		static RendererAPI::API GetAPI()
		{
			return s_RendererAPI->Current();
		}

		static SharedPtr<RendererContext> GetContext()
		{
			return Application::Get().GetWindow().GetRenderContext();
		}

	private:
		static UniquePtr<RendererAPI> s_RendererAPI;
	};
} // namespace Neon
