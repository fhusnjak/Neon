#include "neopch.h"

#include "Renderer/PerspectiveCameraController.h"
#include "VulkanContext.h"
#include "VulkanFramebuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanRendererAPI.h"
#include "VulkanVertexBuffer.h"

#include <imgui/imgui.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace Neon
{
	static std::vector<vk::CommandBuffer> s_ImGuiCommandBuffers;
	static SharedRef<VulkanShader> s_TestShader;
	static SharedRef<VulkanVertexBuffer> s_TestVertexBuffer;
	static SharedRef<VulkanIndexBuffer> s_TestIndexBuffer;
	static SharedRef<VulkanPipeline> s_TestPipeline;
	static SharedRef<VulkanRenderPass> s_TestRenderPass;
	static std::vector<SharedRef<VulkanFramebuffer>> s_TestFramebuffers;

	struct CameraMatrices
	{
		glm::mat4 Model = glm::mat4(1.f);
		glm::mat4 View = glm::mat4(1.f);
		glm::mat4 Projection = glm::mat4(1.f);
	};

	glm::vec3 positions[3] = {glm::vec3(0.f, -0.5f, 0.f), glm::vec3(0.5f, 0.5f, 0.f), glm::vec3(-0.5f, 0.5f, 0.f)};

	uint32 indices[3] = {0, 1, 2};

	VulkanRendererAPI::~VulkanRendererAPI()
	{
	}

	void VulkanRendererAPI::Init()
	{
		s_ImGuiCommandBuffers.resize(VulkanContext::Get()->GetTargetMaxFramesInFlight());
		for (auto& cmdBuff : s_ImGuiCommandBuffers)
		{
			cmdBuff = VulkanContext::GetDevice()->CreateSecondaryCommandBuffer();
		}

		RenderPassSpecification renderPassSpecification;
		s_TestRenderPass = RenderPass::Create(renderPassSpecification).As<VulkanRenderPass>();

		s_TestFramebuffers.resize(VulkanContext::Get()->GetTargetMaxFramesInFlight());
		FramebufferSpecification framebufferSpecification;
		framebufferSpecification.Pass = s_TestRenderPass;
		for (auto& fb : s_TestFramebuffers)
		{
			fb = Framebuffer::Create(framebufferSpecification).As<VulkanFramebuffer>();
		}

		vk::PhysicalDeviceProperties props = VulkanContext::GetDevice()->GetPhysicalDevice()->GetProperties();

		RendererAPI::RenderAPICapabilities& caps = RendererAPI::GetCapabilities();
		caps.Vendor = props.deviceName.operator std::string();
		caps.Renderer = "Vulkan";
		caps.Version = "1.0";

		std::vector<UniformBinding> bindings = {
			{0, UniformType::UniformBuffer, 1, sizeof(CameraMatrices), ShaderStageFlag::Vertex}};

		s_TestShader = Shader::Create(bindings).As<VulkanShader>();
		s_TestShader->LoadShader("C:/VisualStudioProjects/Neon/Neon/src/Shaders/build/test_vert.spv", ShaderType::Vertex);
		s_TestShader->LoadShader("C:/VisualStudioProjects/Neon/Neon/src/Shaders/build/test_frag.spv", ShaderType::Fragment);

		VertexBufferLayout layout({ShaderDataType::Float3});
		s_TestVertexBuffer = VertexBuffer::Create(positions, sizeof(positions), layout).As<VulkanVertexBuffer>();

		s_TestIndexBuffer = IndexBuffer::Create(indices, sizeof(indices)).As<VulkanIndexBuffer>();

		PipelineSpecification pipelineSpecification;
		pipelineSpecification.Shader = s_TestShader;
		pipelineSpecification.Layout = layout;
		pipelineSpecification.Pass = s_TestRenderPass;
		s_TestPipeline = Pipeline::Create(pipelineSpecification).As<VulkanPipeline>();
	}

	void VulkanRendererAPI::Render(SharedRef<PerspectiveCameraController>& camera)
	{
		const VulkanSwapChain& swapChain = VulkanContext::Get()->GetSwapChain();
		uint32 width = swapChain.GetWidth();
		uint32 height = swapChain.GetHeight();

		CameraMatrices cameraMatrices = {};
		cameraMatrices.View = camera->GetCamera().GetViewMatrix();
		cameraMatrices.Projection = camera->GetCamera().GetProjectionMatrix();
		s_TestShader->SetUniformBuffer(0, 0, &cameraMatrices);

		vk::CommandBuffer renderCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

		vk::ClearValue clearValues[2];
		std::array<float, 4> clearColor = {0.2f, 0.2f, 0.2f, 1.0f};
		clearValues[0].color = vk::ClearColorValue(clearColor);
		clearValues[1].depthStencil = {1.0f, 0};

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
		renderPassBeginInfo.framebuffer = s_TestFramebuffers[swapChain.GetCurrentFrameIndex()]->GetHandle();

		renderCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

		// Update dynamic viewport state
		vk::Viewport sceneViewport = {};
		sceneViewport.x = 0.f;
		sceneViewport.y = 0.f;
		sceneViewport.width = (float)width;
		sceneViewport.height = (float)height;
		sceneViewport.minDepth = 0.f;
		sceneViewport.maxDepth = 1.f;

		renderCommandBuffer.setViewport(0, 1, &sceneViewport);

		// Update dynamic scissor state
		vk::Rect2D sceneCcissor = {};
		sceneCcissor.offset.x = 0;
		sceneCcissor.offset.y = 0;
		sceneCcissor.extent.width = width;
		sceneCcissor.extent.height = height;

		renderCommandBuffer.setScissor(0, 1, &sceneCcissor);

		renderCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, s_TestPipeline->GetHandle());
		renderCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_TestPipeline->GetLayout(), 0, 1,
											   &s_TestShader->GetDescriptorSet(), 0, nullptr);
		renderCommandBuffer.bindVertexBuffers(0, {s_TestVertexBuffer->GetHandle()}, {0});
		renderCommandBuffer.bindIndexBuffer(s_TestIndexBuffer->GetHandle(), 0, vk::IndexType::eUint32);
		renderCommandBuffer.drawIndexed(s_TestIndexBuffer->GetCount(), 1, 0, 0, 0);

		renderCommandBuffer.endRenderPass();

		// ImGui Pass
		{
			// Update dynamic viewport state
			vk::Viewport viewport = {};
			viewport.x = 0.f;
			viewport.y = 0.f;
			viewport.width = (float)width;
			viewport.height = (float)height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			renderCommandBuffer.setViewport(0, 1, &viewport);

			// Update dynamic scissor state
			vk::Rect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = width;
			scissor.extent.height = height;

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

	void* VulkanRendererAPI::GetColorImageId()
	{
		const VulkanSwapChain& swapChain = VulkanContext::Get()->GetSwapChain();
		return s_TestFramebuffers[swapChain.GetCurrentFrameIndex()]->GetColorImageID();
	}

	void VulkanRendererAPI::Shutdown()
	{
		VulkanContext::GetDevice()->GetHandle().waitIdle();
		s_ImGuiCommandBuffers.clear();
		s_TestPipeline.Reset();
		s_TestVertexBuffer.Reset();
		s_TestIndexBuffer.Reset();
		s_TestShader.Reset();
		s_TestFramebuffers.clear();
		s_TestRenderPass.Reset();
	}

} // namespace Neon
