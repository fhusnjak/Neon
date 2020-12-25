#pragma once

#include "Neon/Core/Application.h"
#include "RendererAPI.h"
#include "RendererContext.h"

namespace Neon
{
	class Renderer
	{
	public:
		static void Init();

		static void Render();

		static void Shutdown();

		static RendererAPI::API GetAPI()
		{
			return RendererAPI::Current();
		}

		static SharedPtr<RendererContext> GetContext()
		{
			return Application::Get().GetWindow().GetRenderContext();
		}

	private:
		static UniquePtr<RendererAPI> s_RendererAPI;
	};
} // namespace Neon
