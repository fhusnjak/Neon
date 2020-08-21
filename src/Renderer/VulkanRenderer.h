#ifndef NEON_VULKANRENDERER_H
#define NEON_VULKANRENDERER_H

#include "PerspectiveCamera.h"
#include "VulkanShader.h"
#include <Scene/Components.h>
#include <Scene/Entity.h>

#include "DescriptorSet.h"
#include "GraphicsPipeline.h"

#define GLFW_INCLUDE_VULKAN
#include "Window/Window.h"

#include "Allocator.h"
#include "DescriptorPool.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "Window.h"

#define MAX_SWAP_CHAIN_IMAGES 8

namespace Neon
{
struct PushConstant
{
	glm::vec3 cameraPos;
	glm::mat4 view;
	glm::mat4 projection;

	glm::vec4 clippingPlane;

	glm::mat4 model;

	int pointLight;
	float lightIntensity;
	glm::vec3 lightDirection;
	glm::vec3 lightPosition;
};

class VulkanRenderer
{
public:
	VulkanRenderer(const VulkanRenderer& other) = delete;
	VulkanRenderer(const VulkanRenderer&& other) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&&) = delete;
	static void Init(Window* window);
	static void Shutdown();
	static void Begin();
	static void End();
	static void BeginScene(const std::vector<vk::UniqueFramebuffer>& frameBuffers,
						   const glm::vec4& clearColor, const Neon::PerspectiveCamera& camera,
						   const glm::vec4& clippingPlane, bool pointLight, float lightIntensity,
						   glm::vec3 lightDirection, const glm::vec3& lightPosition);
	static void EndScene();
	static void DrawImGui();
	static vk::CommandBuffer BeginSingleTimeCommands();
	static void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);
	static vk::ImageView CreateImageView(vk::Image image, vk::Format format,
										 const vk::ImageAspectFlags& aspectFlags);
	static vk::UniqueImageView CreateImageViewUnique(vk::Image image, vk::Format format,
													 const vk::ImageAspectFlags& aspectFlags);
	static vk::Sampler CreateSampler(const vk::SamplerCreateInfo& createInfo);
	static void* GetOffscreenImageID()
	{
		assert(s_Instance.m_ImGuiOffscreenTextureDescSet);
		return s_Instance.m_ImGuiOffscreenTextureDescSet;
	}
	static vk::Extent2D GetExtent2D()
	{
		assert(s_Instance.m_SwapChain);
		return s_Instance.m_SwapChain->GetExtent();
	}
	static vk::RenderPass GetOffscreenRenderPass()
	{
		assert(s_Instance.m_OffscreenRenderPass.get());
		return s_Instance.m_OffscreenRenderPass.get();
	}
	static vk::DescriptorPool GetDescriptorPool()
	{
		assert(!s_Instance.m_DescriptorPools.empty());
		return s_Instance.m_DescriptorPools[s_Instance.m_DescriptorPools.size() - 1]->GetHandle();
	}
	static vk::SampleCountFlagBits GetMsaaSamples()
	{
		return s_MsaaSamples;
	}
	static const std::vector<vk::UniqueFramebuffer>& GetOffscreenFramebuffers()
	{
		return s_Instance.m_OffscreenFrameBuffers;
	}

	template<typename T>
	static void Render(const Transform& transformComponent, const T& renderer)
	{
		auto& commandBuffer =
			s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get();
		const auto& extent = s_Instance.m_SwapChain->GetExtent();
		vk::Viewport viewport{
			0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height),
			0.0f, 1.0f};
		commandBuffer.setViewport(0, 1, &viewport);
		vk::Rect2D scissor{{0, 0}, extent};
		commandBuffer.setScissor(0, 1, &scissor);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
								   static_cast<vk::Pipeline>(renderer.m_GraphicsPipeline));

		if (renderer.m_DescriptorSets.size() > 0)
		{
			assert(s_Instance.m_SwapChain->GetImageIndex() < renderer.m_DescriptorSets.size());
			commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, renderer.m_GraphicsPipeline.GetLayout(), 0, 1,
				&renderer.m_DescriptorSets[s_Instance.m_SwapChain->GetImageIndex()].Get(), 0,
				nullptr);
		}

		s_Instance.m_PushConstant.model = transformComponent.m_Transform;
		commandBuffer.pushConstants(renderer.m_GraphicsPipeline.GetLayout(),
									vk::ShaderStageFlagBits::eVertex |
										vk::ShaderStageFlagBits::eFragment,
									0, sizeof(PushConstant), &s_Instance.m_PushConstant);

		commandBuffer.bindVertexBuffers(0, {renderer.m_Mesh.m_VertexBuffer->m_Buffer}, {0});
		commandBuffer.bindIndexBuffer(renderer.m_Mesh.m_IndexBuffer->m_Buffer, 0,
									  vk::IndexType::eUint32);

		commandBuffer.drawIndexed(static_cast<uint32_t>(renderer.m_Mesh.m_IndicesCount), 1, 0, 0,
								  0);
	}

private:
	VulkanRenderer() noexcept;
	void InitRenderer(Window* window);
	void WindowResized();
	void IntegrateImGui();
	void CreateOffscreenRenderer();
	void CreateImGuiRenderer();
	void CreateCommandPool();
	void CreateCommandBuffers();

private:
	static VulkanRenderer s_Instance;

	static const auto s_MsaaSamples = vk::SampleCountFlagBits::e8;

	size_t m_DynamicAlignment = -1;

	std::unique_ptr<SwapChain> m_SwapChain;

	vk::UniqueRenderPass m_OffscreenRenderPass;
	vk::UniqueRenderPass m_ImGuiRenderPass;

	// Used for multisampling
	TextureImage m_SampledImage;

	TextureImage m_OffscreenImageAllocation;
	TextureImage m_OffscreenDepthImageAllocation;

	std::unique_ptr<ImageAllocation> m_DepthImageAllocation{};

	std::vector<vk::UniqueFramebuffer> m_OffscreenFrameBuffers;
	std::vector<vk::UniqueFramebuffer> m_ImGuiFrameBuffers;

	vk::UniqueCommandPool m_CommandPool;

	std::vector<std::unique_ptr<DescriptorPool>> m_DescriptorPools;

	std::shared_ptr<DescriptorPool> m_ImGuiDescriptorPool;

	std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

	VkDescriptorSet m_ImGuiOffscreenTextureDescSet = nullptr;

	PushConstant m_PushConstant;
};
} // namespace Neon

#endif //NEON_VULKANRENDERER_H
