#pragma once

#include "ObjModel.h"
#include "PerspectiveCamera.h"
#include "VulkanShader.h"

#include "DescriptorSetType.h"
#include "GraphicsPipeline.h"

#define GLFW_INCLUDE_VULKAN
#include "Window/Window.h"

#include "Tools/VulkanTools.h"

class VulkanRenderer
{
public:
	VulkanRenderer(const VulkanRenderer& other) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;
	VulkanRenderer(const VulkanRenderer&& other) = delete;
	VulkanRenderer& operator=(const VulkanRenderer&&) = delete;
	static void Init(Window* window);
	static void Shutdown();
	static void PushModels(const std::vector<ObjModel>& models_);
	static void PushModel(const ObjModel& model);
	static void CreateSwapChainDependencies();
	static void PushInstances(const std::vector<ObjInstance>& instances_);
	static void PushInstance(const ObjInstance& instance);
	static void Begin();
	static void End();
	static void BeginScene(const PerspectiveCamera& camera);
	static void EndScene();
	static void Rasterize(const glm::vec4& clearColor, const glm::vec3& lightPosition);
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

private:
	VulkanRenderer() noexcept;
	void InitRenderer(Window* window);
	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateDevice();
	void InitAllocator();
	void CreateSwapChain();
	void RecreateSwapChain();
	void IntegrateImGui();
	void CreateSwapChainImageViews();
	void CreateOffscreenRenderer();
	void CreatePostRenderer();
	void CreateImGuiRenderer();
	void CreateOffscreenDescriptorResources();
	void CreatePostDescriptorResources();
	void UpdateOffscreenDescriptorSets();
	void UpdateInstanceResources();
	void UpdatePostDescriptorSets();
	void CreateOffscreenGraphicsPipeline();
	void CreatePostGraphicsPipeline();
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

	vk::UniqueInstance m_Instance;
	vk::UniqueSurfaceKHR m_Surface;
	vk::PhysicalDevice m_PhysicalDevice;
	vk::UniqueDevice m_Device;

	vk::UniqueSwapchainKHR m_SwapChain;
	std::vector<vk::Image> m_SwapChainImages;
	std::vector<vk::UniqueImageView> m_SwapChainImageViews;

	QueueFamilyIndices m_QueueFamilyIndices;

	vk::Queue m_GraphicsQueue;
	vk::Queue m_PresentQueue;

	vk::Format m_SwapChainImageFormat = vk::Format::eUndefined;
	vk::Extent2D m_Extent;

	vk::UniqueRenderPass m_OffscreenRenderPass;
	vk::UniqueRenderPass m_PostRenderPass;
	vk::UniqueRenderPass m_ImGuiRenderPass;

	GraphicsPipeline m_OffscreenGraphicsPipeline;

	GraphicsPipeline m_PostGraphicsPipeline;

	TextureImage m_OffscreenImageAllocation;
	TextureImage m_OffscreenDepthImageAllocation;

	ImageAllocation m_DepthImageAllocation{};
	vk::UniqueImageView m_DepthImageView;
	TextureImage m_SampledImage;

	std::vector<vk::UniqueFramebuffer> m_OffscreenFrameBuffers;
	std::vector<vk::UniqueFramebuffer> m_PostFrameBuffers;
	std::vector<vk::UniqueFramebuffer> m_ImGuiFrameBuffers;

	vk::UniqueCommandPool m_CommandPool;

	std::vector<BufferAllocation> m_CameraBufferAllocations;

	vk::UniqueDescriptorPool m_OffscreenDescriptorPool;
	DescriptorSetType m_OffscreenDescriptorSets;

	vk::UniqueDescriptorPool m_PostDescriptorPool;
	DescriptorSetType m_PostDescriptorSets;

	vk::UniqueDescriptorPool m_ImGuiDescriptorPool;

	std::vector<vk::UniqueCommandBuffer> m_CommandBuffers;

	std::vector<vk::UniqueSemaphore> m_ImageAvailableSemaphores;
	std::vector<vk::UniqueSemaphore> m_RenderFinishedSemaphores;
	std::vector<vk::UniqueFence> m_InFlightFences;
	std::vector<vk::Fence> m_ImagesInFlight;

	VkDescriptorSet m_ImGuiOffscreenTextureDescSet = nullptr;
};
