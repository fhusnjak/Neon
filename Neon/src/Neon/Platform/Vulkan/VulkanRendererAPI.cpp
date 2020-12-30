#include "neopch.h"

#include "VulkanContext.h"
#include "VulkanFramebuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanRendererAPI.h"
#include "VulkanVertexBuffer.h"

#include <imgui/imgui.h>

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

namespace Neon
{
	static vk::CommandBuffer s_ImGuiCommandBuffers[10];
	static SharedRef<VulkanShader> s_TestShader;
	static SharedRef<VulkanVertexBuffer> s_TestVertexBuffer;
	static SharedRef<VulkanIndexBuffer> s_TestIndexBuffer;
	static SharedRef<VulkanPipeline> s_TestPipeline;
	static SharedRef<VulkanRenderPass> s_TestRenderPass;
	static SharedRef<VulkanFramebuffer> s_TestFramebuffer;

	struct TestUBO
	{
		float color;
	};

	glm::vec3 positions[3] = {glm::vec3(0.f, -0.5f, 0.f), glm::vec3(0.5f, 0.5f, 0.f), glm::vec3(-0.5f, 0.5f, 0.f)};

	uint32 indices[3] = {0, 1, 2};

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

		std::vector<UniformBinding> bindings = {{0, UniformType::UniformBuffer, 2, sizeof(TestUBO), ShaderStageFlag::Fragment}};

		s_TestShader = Shader::Create(bindings).As<VulkanShader>();
		s_TestShader->LoadShader("C:/VisualStudioProjects/Neon/Neon/src/Shaders/build/test_vert.spv", ShaderType::Vertex);
		s_TestShader->LoadShader("C:/VisualStudioProjects/Neon/Neon/src/Shaders/build/test_frag.spv", ShaderType::Fragment);

		VertexBufferLayout layout({ShaderDataType::Float3});
		s_TestVertexBuffer = VertexBuffer::Create(positions, sizeof(positions), layout).As<VulkanVertexBuffer>();

		s_TestIndexBuffer = IndexBuffer::Create(indices, sizeof(indices)).As<VulkanIndexBuffer>();

		RenderPassSpecification renderPassSpecification;
		s_TestRenderPass = RenderPass::Create(renderPassSpecification).As<VulkanRenderPass>();

		FramebufferSpecification framebufferSpecification;
		framebufferSpecification.Pass = s_TestRenderPass;
		s_TestFramebuffer = Framebuffer::Create(framebufferSpecification).As<VulkanFramebuffer>();

		PipelineSpecification pipelineSpecification;
		pipelineSpecification.Shader = s_TestShader;
		pipelineSpecification.Layout = layout;
		pipelineSpecification.Pass = s_TestRenderPass;
		s_TestPipeline = Pipeline::Create(pipelineSpecification).As<VulkanPipeline>();

		TestUBO test[2] = {};
		test[0].color = 1.f;
		test[1].color = 1.f;
		s_TestShader->SetUniformBuffer(0, 0, &test[0]);
		s_TestShader->SetUniformBuffer(0, 1, &test[1]);
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

		vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		renderCommandBuffer.begin(beginInfo);

		vk::RenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.renderPass = s_TestRenderPass->GetHandle();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = s_TestFramebuffer->GetHandle();

		renderCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

		// Update dynamic viewport state
		vk::Viewport viewport = {};
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		renderCommandBuffer.setViewport(0, 1, &viewport);

		// Update dynamic scissor state
		vk::Rect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = width;
		scissor.extent.height = height;

		renderCommandBuffer.setScissor(0, 1, &scissor);

		renderCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, s_TestPipeline->GetHandle());
		renderCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_TestPipeline->GetLayout(), 0, 1,
											   &s_TestShader->GetDescriptorSet(), 0, nullptr);
		renderCommandBuffer.bindVertexBuffers(0, {s_TestVertexBuffer->GetHandle()}, {0});
		renderCommandBuffer.bindIndexBuffer(s_TestIndexBuffer->GetHandle(), 0, vk::IndexType::eUint32);
		renderCommandBuffer.drawIndexed(s_TestIndexBuffer->GetCount(), 1, 0, 0, 0);

		renderCommandBuffer.endRenderPass();

		ImGui::Begin("Viewport");
		ImGui::Image(s_TestFramebuffer->GetColorImageID(), ImVec2{static_cast<float>(width), static_cast<float>(height)});
		ImGui::End();

		ImGui::End();

		// ImGui Pass
		{
			vk::RenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			renderCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eSecondaryCommandBuffers);

			vk::CommandBufferInheritanceInfo inheritanceInfo = {};
			inheritanceInfo.renderPass = swapChain.GetRenderPass();
			inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();
			std::vector<vk::CommandBuffer> commandBuffers;

			vk::CommandBufferBeginInfo beginInfo = {};
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
			beginInfo.pInheritanceInfo = &inheritanceInfo;

			s_ImGuiCommandBuffers[swapChain.GetCurrentFrameIndex()].begin(beginInfo);

			s_ImGuiCommandBuffers[swapChain.GetCurrentFrameIndex()].setViewport(0, 1, &viewport);

			s_ImGuiCommandBuffers[swapChain.GetCurrentFrameIndex()].setScissor(0, 1, &scissor);

			// TODO: Move to VulkanImGuiLayer
			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), s_ImGuiCommandBuffers[swapChain.GetCurrentFrameIndex()]);

			s_ImGuiCommandBuffers[swapChain.GetCurrentFrameIndex()].end();

			commandBuffers.push_back(s_ImGuiCommandBuffers[swapChain.GetCurrentFrameIndex()]);

			renderCommandBuffer.executeCommands(commandBuffers);

			renderCommandBuffer.endRenderPass();
		}

		renderCommandBuffer.end();
	}

	void VulkanRendererAPI::Shutdown()
	{
		VulkanContext::GetDevice()->GetHandle().waitIdle();
		for (auto& cmdBuff : s_ImGuiCommandBuffers)
		{
			VulkanContext::GetDevice()->GetHandle().freeCommandBuffers(VulkanContext::GetDevice()->GetCommandPool(), cmdBuff);
		}
		s_TestPipeline.Reset();
		s_TestVertexBuffer.Reset();
		s_TestShader.Reset();
	}

} // namespace Neon
