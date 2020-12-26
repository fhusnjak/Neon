#include "neopch.h"

#include "VulkanContext.h"
#include "VulkanRendererAPI.h"

#include <imgui/imgui.h>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

namespace Neon
{
	static vk::CommandBuffer s_ImGuiCommandBuffers[10];

	VulkanRendererAPI::~VulkanRendererAPI()
	{
	}

	void VulkanRendererAPI::Init()
	{
		for (auto& cmdBuff : s_ImGuiCommandBuffers)
		{
			cmdBuff = VulkanContext::GetDevice()->CreateSecondaryCommandBuffer();
		}

		vk::PhysicalDeviceProperties props = VulkanContext::GetDevice()->GetPhysicalDevice()->GetProperties();

		RendererAPI::RenderAPICapabilities& caps = RendererAPI::GetCapabilities();
		caps.Vendor = props.deviceName;
		caps.Renderer = "Vulkan";
		caps.Version = "1.0";
	}

	void VulkanRendererAPI::Render()
	{
		const VulkanSwapChain& swapChain = VulkanContext::Get()->GetSwapChain();
		vk::CommandBuffer renderCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

		vk::ClearValue clearValues[2];
		std::array<float, 4> clearColor = {0.2f, 0.2f, 0.2f, 1.0f};
		clearValues[0].color = vk::ClearColorValue(clearColor);
		clearValues[1].depthStencil = {1.0f, 0};

		uint32_t width = swapChain.GetWidth();
		uint32_t height = swapChain.GetHeight();

		vk::RenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

		vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		renderCommandBuffer.begin(beginInfo);
		renderCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

		vk::CommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.renderPass = swapChain.GetRenderPass();
		inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();
		std::vector<vk::CommandBuffer> commandBuffers;
		// ImGui Pass
		{
			vk::CommandBufferBeginInfo beginInfo = {};
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
			beginInfo.pInheritanceInfo = &inheritanceInfo;

			s_ImGuiCommandBuffers[swapChain.GetCurrentBufferIndex()].begin(beginInfo);

			// Update dynamic viewport state
			vk::Viewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = (float)height;
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			s_ImGuiCommandBuffers[swapChain.GetCurrentBufferIndex()].setViewport(0, 1, &viewport);

			// Update dynamic scissor state
			vk::Rect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;

			s_ImGuiCommandBuffers[swapChain.GetCurrentBufferIndex()].setScissor(0, 1, &scissor);

			// TODO: Move to VulkanImGuiLayer
			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), s_ImGuiCommandBuffers[swapChain.GetCurrentBufferIndex()]);

			s_ImGuiCommandBuffers[swapChain.GetCurrentBufferIndex()].end();

			commandBuffers.push_back(s_ImGuiCommandBuffers[swapChain.GetCurrentBufferIndex()]);
		}

		renderCommandBuffer.executeCommands(commandBuffers);

		renderCommandBuffer.endRenderPass();
		renderCommandBuffer.end();
	}

	void VulkanRendererAPI::Shutdown()
	{
		VulkanContext::GetDevice()->GetHandle().waitIdle();
		for (auto& cmdBuff : s_ImGuiCommandBuffers)
		{
			VulkanContext::GetDevice()->GetHandle().freeCommandBuffers(VulkanContext::GetDevice()->GetCommandPool(),
																	   cmdBuff);
		}	
	}

} // namespace Neon
