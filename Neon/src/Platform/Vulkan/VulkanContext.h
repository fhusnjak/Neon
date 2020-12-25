#pragma once

#include "Neon/Renderer/Renderer.h"
#include "Neon/Renderer/RendererContext.h"

#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

struct GLFWwindow;

namespace Neon
{
	class VulkanContext : public RendererContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual ~VulkanContext() = default;

		void BeginFrame() override;
		void SwapBuffers() override;

		void OnResize(uint32 width, uint32 height) override;

		SharedPtr<VulkanDevice> GetDevice() const
		{
			return m_Device;
		}

		const VulkanSwapChain& GetSwapChain() const
		{
			return m_SwapChain;
		}

		static vk::Instance GetInstance()
		{
			return s_Instance.get();
		}

		static SharedPtr<VulkanContext> Get()
		{
			return std::dynamic_pointer_cast<VulkanContext>(Renderer::GetContext());
		}

	private:
		GLFWwindow* m_WindowHandle;

		inline static vk::UniqueInstance s_Instance;

		vk::DebugReportCallbackEXT m_DebugReportCallback;

		SharedPtr<VulkanPhysicalDevice> m_PhysicalDevice;
		SharedPtr<VulkanDevice> m_Device;

		VulkanSwapChain m_SwapChain;

		const std::vector<const char*> m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
		std::vector<const char*> m_InstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME,
														 VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
														 VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
	};

} // namespace Neon
