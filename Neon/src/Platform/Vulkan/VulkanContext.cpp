#include "neopch.h"

#include "VulkanContext.h"

#include <GLFW/glfw3.h>

#define IMGUI_IMPL_API
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

namespace Neon
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags,
																	VkDebugReportObjectTypeEXT objectType, uint64_t object,
																	size_t location, int32_t messageCode, const char* pLayerPrefix,
																	const char* pMessage, void* pUserData)
	{
		NEO_CORE_WARN("VulkanDebugCallback:\n  Object Type: {0}\n  Message: {1}", objectType, pMessage);
		return VK_FALSE;
	}

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		NEO_CORE_ASSERT(glfwVulkanSupported(), "Vulkan is not supported");

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		m_InstanceExtensions.insert(m_InstanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);

		std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();
		for (const auto& extension : m_InstanceExtensions)
		{
			bool found = false;
			for (const auto& supportedExtension : supportedExtensions)
			{
				if (strcmp(supportedExtension.extensionName, extension) != 0)
				{
					found = true;
					break;
				}
			}
			NEO_CORE_ASSERT(found, "Not all required instance extensions are supported");
		}

		vk::ApplicationInfo applicationInfo("Neon", 1, "Vulkan engine", 1, VK_API_VERSION_1_0);
#ifdef NEO_DEBUG
		vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, static_cast<uint32>(m_ValidationLayers.size()),
												  m_ValidationLayers.data(), static_cast<uint32>(m_InstanceExtensions.size()),
												  m_InstanceExtensions.data());
#else
		vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, 0, nullptr,
												  static_cast<uint32_t>(m_InstanceExtensions.size()), m_InstanceExtensions.data());
#endif
		s_Instance = vk::createInstanceUnique(instanceCreateInfo);

		vk::DebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
		debugReportCreateInfo.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |
									  vk::DebugReportFlagBitsEXT::ePerformanceWarning;
		debugReportCreateInfo.pfnCallback = VulkanDebugReportCallback;

		vk::DispatchLoaderDynamic dldi(s_Instance.get(), vkGetInstanceProcAddr);
		m_DebugReportCallback = s_Instance.get().createDebugReportCallbackEXT(debugReportCreateInfo, nullptr, dldi);

		m_PhysicalDevice = VulkanPhysicalDevice::Select();
		m_Device = VulkanDevice::Create(m_PhysicalDevice);

		m_SwapChain.Init(s_Instance.get(), m_Device);
		m_SwapChain.InitSurface(m_WindowHandle);
		// This window size should be ignored
		uint32_t width = 1920, height = 1080;
		m_SwapChain.Create(&width, &height);
	}

	void VulkanContext::BeginFrame()
	{
		m_SwapChain.BeginFrame();
	}

	void VulkanContext::SwapBuffers()
	{
		vk::CommandBuffer drawCommandBuffer = m_SwapChain.GetCurrentDrawCommandBuffer();

		// TODO: Temporary

		vk::ClearValue clearValues[2];
		std::array<float, 4> clearColor = {0.2f, 0.2f, 0.2f, 1.0f};
		clearValues[0].color = vk::ClearColorValue(clearColor);
		clearValues[1].depthStencil = {1.0f, 0};

		uint32_t width = m_SwapChain.GetWidth();
		uint32_t height = m_SwapChain.GetHeight();

		vk::RenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.renderPass = m_SwapChain.GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = m_SwapChain.GetCurrentFramebuffer();

		vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		drawCommandBuffer.begin(beginInfo);
		drawCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

		// ImGui Pass
		{
			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), drawCommandBuffer);
		}

		drawCommandBuffer.endRenderPass();
		drawCommandBuffer.end();

		m_SwapChain.Present();
	}

	void VulkanContext::OnResize(uint32 width, uint32 height)
	{
		m_SwapChain.OnResize(width, height);
	}
} // namespace Neon
