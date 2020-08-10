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
class VulkanRenderer
{
public:
	VulkanRenderer(const VulkanRenderer& other) = delete;
	VulkanRenderer(const VulkanRenderer&& other) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&&) = delete;
	static void Init(Window* window);
	static void Shutdown();
	static void CreateSwapChainDependencies();
	static void Begin();
	static void End();
	static void BeginScene(const PerspectiveCamera& camera, const glm::vec4& clearColor);
	static void EndScene();
	static void Render(const TransformComponent& transformComponent,
					   const MeshComponent& meshComponent,
					   const MaterialComponent& materialComponent, const glm::vec3& lightPosition);
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
	static void CreateWavefrontDescriptorSet(MaterialComponent& materialComponent);

private:
	VulkanRenderer() noexcept;
	void InitRenderer(Window* window);
	void CreateSurface();
	void CreateSwapChain();
	void WindowResized();
	void IntegrateImGui();
	void CreateOffscreenRenderer();
	void CreateImGuiRenderer();
	void CreateOffscreenGraphicsPipeline();
	void CreateCommandPool();
	void CreateUniformBuffers(std::vector<BufferAllocation>& bufferAllocations,
							  vk::DeviceSize bufferSize);
	void CreateCommandBuffers();
	void CreateSyncObjects();
	void UpdateCameraMatrices(const PerspectiveCamera& camera);

private:
	static VulkanRenderer s_Instance;
	static uint32_t s_ImageIndex;
	static uint32_t s_CurrentFrame;

	Window* m_Window = nullptr;

	size_t m_DynamicAlignment = -1;

	vk::UniqueSurfaceKHR m_Surface;

	std::unique_ptr<SwapChain> m_SwapChain;

	vk::Extent2D m_Extent;

	vk::UniqueRenderPass m_OffscreenRenderPass;
	vk::UniqueRenderPass m_ImGuiRenderPass;

	GraphicsPipeline m_OffscreenGraphicsPipeline;

	TextureImage m_OffscreenImageAllocation;
	TextureImage m_OffscreenDepthImageAllocation;

	ImageAllocation m_DepthImageAllocation{};
	TextureImage m_SampledImage;

	std::vector<vk::UniqueFramebuffer> m_OffscreenFrameBuffers;
	std::vector<vk::UniqueFramebuffer> m_ImGuiFrameBuffers;

	vk::UniqueCommandPool m_CommandPool;

	std::vector<BufferAllocation> m_CameraBufferAllocations;

	std::vector<std::shared_ptr<DescriptorPool>> m_DescriptorPools;
	vk::DescriptorSetLayout m_WavefrontLayout;

	std::shared_ptr<DescriptorPool> m_ImGuiDescriptorPool;

	std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

	std::vector<vk::UniqueSemaphore> m_ImageAvailableSemaphores;
	std::vector<vk::UniqueSemaphore> m_RenderFinishedSemaphores;
	std::vector<vk::UniqueFence> m_InFlightFences;
	std::vector<vk::Fence> m_ImagesInFlight;

	VkDescriptorSet m_ImGuiOffscreenTextureDescSet = nullptr;
};
} // namespace Neon

#endif //NEON_VULKANRENDERER_H
