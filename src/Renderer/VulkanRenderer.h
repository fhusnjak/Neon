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

namespace Neon
{

struct CameraMatrices
{
	glm::vec3 cameraPos;
	glm::mat4 view;
	glm::mat4 projection;
};

struct PushConstant
{
	glm::mat4 transformComponent;
	float lightIntensity;
	[[maybe_unused]] glm::vec3 lightPosition;
	[[maybe_unused]] glm::vec3 lightColor;
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
	static void BeginScene(const PerspectiveCamera& camera, const glm::vec4& clearColor);
	static void EndScene();
	static void DrawImGui();
	static vk::CommandBuffer BeginSingleTimeCommands();
	static void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);
	static vk::ImageView CreateImageView(vk::Image image, vk::Format format,
										 const vk::ImageAspectFlags& aspectFlags);
	static vk::UniqueImageView CreateImageViewUnique(vk::Image image, vk::Format format,
													 const vk::ImageAspectFlags& aspectFlags);
	static vk::Sampler CreateSampler(const vk::SamplerCreateInfo& createInfo);
	static void* GetOffscreenImageID();
	static vk::Extent2D GetExtent2D();
	static void LoadSkyDome(SkyDomeRenderer& skyDomeRenderer);
	static void LoadModel(MeshRenderer& meshRenderer);
	static void LoadAnimatedModel(SkinnedMeshRenderer& meshComponent);

	template <typename T>
	static void Render(const Transform& transformComponent,
					   const T& renderer, float lightIntensity, const glm::vec3& lightPosition)
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

		std::vector<vk::DescriptorSet> descriptorSets = {
			s_Instance.m_CameraDescriptorSets[s_Instance.m_SwapChain->GetImageIndex()].Get()};
		if (s_Instance.m_SwapChain->GetImageIndex() < renderer.m_DescriptorSets.size())
		{
			descriptorSets.push_back(renderer.m_DescriptorSets[s_Instance.m_SwapChain->GetImageIndex()].Get());
		}

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
										 renderer.m_GraphicsPipeline.GetLayout(), 0,
										 descriptorSets.size(), descriptorSets.data(), 0, nullptr);

		PushConstant pushConstant{transformComponent.m_Transform, lightIntensity, lightPosition, {1, 0, 1}};
		commandBuffer.pushConstants(renderer.m_GraphicsPipeline.GetLayout(),
									vk::ShaderStageFlagBits::eVertex |
									vk::ShaderStageFlagBits::eFragment,
									0, sizeof(PushConstant), &pushConstant);

		commandBuffer.bindVertexBuffers(0, {renderer.m_Mesh.m_VertexBuffer->buffer}, {0});
		commandBuffer.bindIndexBuffer(renderer.m_Mesh.m_IndexBuffer->buffer, 0, vk::IndexType::eUint32);

		commandBuffer.drawIndexed(static_cast<uint32_t>(renderer.m_Mesh.m_IndicesCount), 1, 0, 0, 0);
	}

private:
	VulkanRenderer() noexcept;
	void InitRenderer(Window* window);
	void WindowResized();
	void IntegrateImGui();
	void CreateOffscreenRenderer();
	void CreateImGuiRenderer();
	void CreateCommandPool();
	void CreateUniformBuffers(std::vector<std::unique_ptr<BufferAllocation>>& bufferAllocations,
							  vk::DeviceSize bufferSize);
	void CreateCommandBuffers();
	void UpdateCameraMatrices(const PerspectiveCamera& camera);

private:
	static VulkanRenderer s_Instance;

	std::vector<DescriptorSet> m_CameraDescriptorSets;

	size_t m_DynamicAlignment = -1;

	std::unique_ptr<SwapChain> m_SwapChain;

	vk::UniqueRenderPass m_OffscreenRenderPass;
	vk::UniqueRenderPass m_ImGuiRenderPass;

	TextureImage m_SampledImage;

	TextureImage m_OffscreenImageAllocation;
	TextureImage m_OffscreenDepthImageAllocation;

	std::unique_ptr<ImageAllocation> m_DepthImageAllocation{};

	std::vector<vk::UniqueFramebuffer> m_OffscreenFrameBuffers;
	std::vector<vk::UniqueFramebuffer> m_ImGuiFrameBuffers;

	vk::UniqueCommandPool m_CommandPool;

	std::vector<std::unique_ptr<BufferAllocation>> m_CameraBufferAllocations;

	std::vector<std::unique_ptr<DescriptorPool>> m_DescriptorPools;

	std::shared_ptr<DescriptorPool> m_ImGuiDescriptorPool;

	std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

	VkDescriptorSet m_ImGuiOffscreenTextureDescSet = nullptr;
};
} // namespace Neon

#endif //NEON_VULKANRENDERER_H
