#pragma once

namespace Neon
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			Vulkan = 1
		};

		struct RenderAPICapabilities
		{
			std::string Vendor;
			std::string Renderer;
			std::string Version;

			int MaxSamples = 0;
			float MaxAnisotropy = 0.0f;
			int MaxTextureUnits = 0;
		};

	public:
		virtual ~RendererAPI() = default;

		static API Current()
		{
			return s_API;
		}
		static UniquePtr<RendererAPI> Create();

	private:
		static API s_API;
	};
} // namespace Neon
