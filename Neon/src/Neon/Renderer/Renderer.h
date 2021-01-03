#pragma once

#include "Neon/Core/Application.h"
#include "RendererAPI.h"
#include "RendererContext.h"
#include "PerspectiveCameraController.h"

namespace Neon
{
	class Renderer
	{
	public:
		static void Init();

		static void Render(const SharedRef<PerspectiveCameraController>& camera);

		static void Shutdown();

		static RendererAPI::API GetAPI()
		{
			return RendererAPI::Current();
		}

		static SharedRef<RendererContext> GetContext()
		{
			return Application::Get().GetWindow().GetRenderContext();
		}

	private:
		static UniqueRef<RendererAPI> s_RendererAPI;
	};
} // namespace Neon
