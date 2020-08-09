#include "VulkanRenderer.h"

#include "Allocator.h"
#include "Context.h"
#include "DescriptorPool.h"
#include "RenderPass.h"
#include "Window.h"

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 1

#define MAX_SWAP_CHAIN_IMAGES 8

#define MAX_DESCRIPTOR_SETS_PER_POOL 1024

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

struct CameraMatrices
{
	glm::vec3 cameraPos;
	glm::mat4 view;
	glm::mat4 projection;
};

struct PushConstant
{
	glm::mat4 transformComponent;
	glm::vec3 lightPosition;
	glm::vec3 lightColor;
};

auto msaaSamples = vk::SampleCountFlagBits::e8;

Neon::VulkanRenderer Neon::VulkanRenderer::s_Instance;
uint32_t Neon::VulkanRenderer::s_ImageIndex = 0;
uint32_t Neon::VulkanRenderer::s_CurrentFrame = 0;

static std::vector<const char*> instanceExtensions;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-equals-default"
Neon::VulkanRenderer::VulkanRenderer() noexcept { }
#pragma clang diagnostic pop

void Neon::VulkanRenderer::Init(Window* window)
{
	instanceExtensions.push_back({VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME});
	s_Instance.InitRenderer(window);
}

void Neon::VulkanRenderer::Shutdown()
{
	Neon::Context::GetInstance().GetLogicalDevice().GetHandle().waitIdle();
}

void Neon::VulkanRenderer::CreateSwapChainDependencies()
{
	s_Instance.CreateOffscreenGraphicsPipeline();
	s_Instance.IntegrateImGui();
}

void Neon::VulkanRenderer::Begin()
{
	auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	logicalDevice.waitForFences(s_Instance.m_InFlightFences[s_CurrentFrame].get(), VK_TRUE,
								UINT64_MAX);
	auto result = logicalDevice.acquireNextImageKHR(
		s_Instance.m_SwapChain.get(), UINT64_MAX,
		s_Instance.m_ImageAvailableSemaphores[s_CurrentFrame].get(), nullptr);
	s_ImageIndex = result.value;

	if (result.result == vk::Result::eErrorOutOfDateKHR)
	{
		s_Instance.RecreateSwapChain();
		return;
	}
	else
	{
		assert(result.result == vk::Result::eSuccess ||
			   result.result == vk::Result::eSuboptimalKHR);
	}
	if (s_Instance.m_ImagesInFlight[s_ImageIndex] != vk::Fence())
	{
		logicalDevice.waitForFences(s_Instance.m_ImagesInFlight[s_CurrentFrame], VK_TRUE,
									UINT64_MAX);
	}
	s_Instance.m_ImagesInFlight[s_ImageIndex] = s_Instance.m_InFlightFences[s_CurrentFrame].get();

	vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	s_Instance.m_CommandBuffers[s_ImageIndex].get().begin(beginInfo);
}

void Neon::VulkanRenderer::End()
{
	const auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice();
	s_Instance.m_CommandBuffers[s_ImageIndex].get().end();

	vk::Semaphore waitSemaphores[] = {s_Instance.m_ImageAvailableSemaphores[s_CurrentFrame].get()};
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::Semaphore signalSemaphores[] = {
		s_Instance.m_RenderFinishedSemaphores[s_CurrentFrame].get()};
	vk::SubmitInfo submitInfo{
		1, waitSemaphores,	waitStages, 1, &s_Instance.m_CommandBuffers[s_ImageIndex].get(),
		1, signalSemaphores};

	logicalDevice.GetHandle().resetFences({s_Instance.m_InFlightFences[s_CurrentFrame].get()});

	logicalDevice.GetGraphicsQueue().submit({submitInfo},
											s_Instance.m_InFlightFences[s_CurrentFrame].get());

	vk::PresentInfoKHR presentInfo{1, signalSemaphores, 1, &s_Instance.m_SwapChain.get(),
								   &s_ImageIndex};

	auto result = logicalDevice.GetPresentQueue().presentKHR(&presentInfo);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
		s_Instance.m_Window->Resized())
	{
		s_Instance.m_Window->ResetResized();
		s_Instance.RecreateSwapChain();
	}
	else
	{
		assert(result == vk::Result::eSuccess);
	}

	//s_CurrentFrame = (s_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Neon::VulkanRenderer::BeginScene(const Neon::PerspectiveCamera& camera,
									  const glm::vec4& clearColor)
{
	s_Instance.UpdateCameraMatrices(camera);
	std::array<vk::ClearValue, 3> clearValues = {};
	memcpy(&clearValues[0].color.float32, &clearColor, sizeof(clearValues[0].color.float32));
	clearValues[1].depthStencil = VkClearDepthStencilValue{1.0f, 0};
	memcpy(&clearValues[2].color.float32, &clearColor, sizeof(clearValues[0].color.float32));
	vk::RenderPassBeginInfo renderPassInfo{s_Instance.m_OffscreenRenderPass.get(),
										   s_Instance.m_OffscreenFrameBuffers[s_ImageIndex].get(),
										   {{0, 0}, s_Instance.m_Extent},
										   static_cast<uint32_t>(clearValues.size()),
										   clearValues.data()};
	s_Instance.m_CommandBuffers[s_ImageIndex].get().beginRenderPass(renderPassInfo,
																	vk::SubpassContents::eInline);
}

void Neon::VulkanRenderer::EndScene()
{
	s_Instance.m_CommandBuffers[s_ImageIndex].get().endRenderPass();
}

void Neon::VulkanRenderer::Render(const TransformComponent& transformComponent,
								  const MeshComponent& meshComponent,
								  const MaterialComponent& materialComponent,
								  const glm::vec3& lightPosition)
{
	s_Instance.m_CommandBuffers[s_ImageIndex].get().bindPipeline(
		vk::PipelineBindPoint::eGraphics, s_Instance.m_OffscreenGraphicsPipeline);

	s_Instance.m_CommandBuffers[s_ImageIndex].get().bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, s_Instance.m_OffscreenGraphicsPipeline.GetLayout(), 0, 1,
		&materialComponent.m_DescriptorSets[s_ImageIndex].Get(), 0, nullptr);

	PushConstant pushConstant{transformComponent, lightPosition, {1, 0, 1}};
	s_Instance.m_CommandBuffers[s_ImageIndex].get().pushConstants(
		s_Instance.m_OffscreenGraphicsPipeline.GetLayout(),
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
		sizeof(PushConstant), &pushConstant);

	s_Instance.m_CommandBuffers[s_ImageIndex].get().bindVertexBuffers(
		0, {meshComponent.m_VertexBuffer.buffer}, {0});
	s_Instance.m_CommandBuffers[s_ImageIndex].get().bindIndexBuffer(
		meshComponent.m_IndexBuffer.buffer, 0, vk::IndexType::eUint32);

	s_Instance.m_CommandBuffers[s_ImageIndex].get().drawIndexed(
		static_cast<uint32_t>(meshComponent.m_IndicesCount), 1, 0, 0, 0);
}

void Neon::VulkanRenderer::DrawImGui()
{
	vk::RenderPassBeginInfo renderPassInfo{s_Instance.m_ImGuiRenderPass.get(),
										   s_Instance.m_ImGuiFrameBuffers[s_ImageIndex].get(),
										   {{0, 0}, s_Instance.m_Extent}};
	s_Instance.m_CommandBuffers[s_ImageIndex].get().beginRenderPass(renderPassInfo,
																	vk::SubpassContents::eInline);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
									s_Instance.m_CommandBuffers[s_ImageIndex].get());

	s_Instance.m_CommandBuffers[s_ImageIndex].get().endRenderPass();
}

vk::CommandBuffer Neon::VulkanRenderer::BeginSingleTimeCommands()
{
	auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	vk::CommandBufferAllocateInfo allocInfo{s_Instance.m_CommandPool.get(),
											vk::CommandBufferLevel::ePrimary, 1};
	vk::CommandBuffer commandBuffer = logicalDevice.allocateCommandBuffers(allocInfo)[0];
	vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	commandBuffer.begin(beginInfo);
	return commandBuffer;
}

void Neon::VulkanRenderer::EndSingleTimeCommands(vk::CommandBuffer commandBuffer)
{
	const auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice();
	commandBuffer.end();
	vk::SubmitInfo submitInfo{0, nullptr, nullptr, 1, &commandBuffer};
	logicalDevice.GetGraphicsQueue().submit(submitInfo, nullptr);
	logicalDevice.GetGraphicsQueue().waitIdle();
	logicalDevice.GetHandle().freeCommandBuffers(s_Instance.m_CommandPool.get(), commandBuffer);
}

vk::ImageView Neon::VulkanRenderer::CreateImageView(vk::Image image, vk::Format format,
													const vk::ImageAspectFlags& aspectFlags)
{
	auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	vk::ImageSubresourceRange subResourceRange(aspectFlags, 0, 1, 0, 1);
	vk::ImageViewCreateInfo imageViewCreateInfo{{},		image, vk::ImageViewType::e2D,
												format, {},	   subResourceRange};
	return logicalDevice.createImageView(imageViewCreateInfo);
}

vk::UniqueImageView
Neon::VulkanRenderer::CreateImageViewUnique(vk::Image image, vk::Format format,
											const vk::ImageAspectFlags& aspectFlags)
{
	auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	vk::ImageSubresourceRange subResourceRange(aspectFlags, 0, 1, 0, 1);
	vk::ImageViewCreateInfo imageViewCreateInfo{{},		image, vk::ImageViewType::e2D,
												format, {},	   subResourceRange};
	return logicalDevice.createImageViewUnique(imageViewCreateInfo);
}

vk::Sampler Neon::VulkanRenderer::CreateSampler(const vk::SamplerCreateInfo& createInfo)
{
	return Neon::Context::GetInstance().GetLogicalDevice().GetHandle().createSampler(createInfo);
}

void Neon::VulkanRenderer::InitRenderer(Window* window)
{
	//TODO: swap chain image size should not effect number of descriptor sets and uniform buffers
	m_Window = window;
	Neon::Context::GetInstance().Init();
	CreateSurface();
	Neon::Context::GetInstance().CreateDevice(m_Surface.get(), {vk::QueueFlagBits::eGraphics});
	Neon::Context::GetInstance().InitAllocator();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateCommandPool();
	CreateOffscreenRenderer();
	CreateImGuiRenderer();
	CreateCommandBuffers();
	CreateSyncObjects();
	s_Instance.CreateUniformBuffers(s_Instance.m_CameraBufferAllocations, sizeof(CameraMatrices));
	CreateSwapChainDependencies();
	Neon::Context::GetInstance().GetLogicalDevice().GetHandle().waitIdle();
}

void Neon::VulkanRenderer::CreateSurface()
{
	VkSurfaceKHR surface;
	glfwCreateWindowSurface(Neon::Context::GetInstance().GetVkInstance(),
							m_Window->GetNativeWindow(), nullptr, &surface);
	m_Surface =
		vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface), Neon::Context::GetInstance().GetVkInstance());
}

void Neon::VulkanRenderer::CreateSwapChain()
{
	const auto& physicalDevice = Neon::Context::GetInstance().GetPhysicalDevice();
	const auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice();
	auto deviceSurfaceProperties = physicalDevice.GetDeviceSurfaceProperties(m_Surface.get());
	std::vector<vk::SurfaceFormatKHR> availableSurfaceFormats = deviceSurfaceProperties.formats;
	assert(!availableSurfaceFormats.empty());
	vk::SurfaceFormatKHR surfaceFormat = availableSurfaceFormats[0];
	for (const auto& availableSurfaceFormat : availableSurfaceFormats)
	{
		if (availableSurfaceFormat.format == vk::Format::eB8G8R8A8Unorm &&
			availableSurfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{ surfaceFormat = availableSurfaceFormat; }
	}
	m_SwapChainImageFormat = surfaceFormat.format;

	std::vector<vk::PresentModeKHR> availablePresentModes = deviceSurfaceProperties.presentModes;
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == vk::PresentModeKHR::eMailbox)
		{ presentMode = availablePresentMode; }
	}

	if (deviceSurfaceProperties.surfaceCapabilities.currentExtent.width != UINT32_MAX)
	{ m_Extent = deviceSurfaceProperties.surfaceCapabilities.currentExtent; }
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_Window->GetNativeWindow(), &width, &height);
		m_Extent = vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		m_Extent.width = max(
			deviceSurfaceProperties.surfaceCapabilities.minImageExtent.width,
			min(deviceSurfaceProperties.surfaceCapabilities.maxImageExtent.width, m_Extent.width));
		m_Extent.height = max(deviceSurfaceProperties.surfaceCapabilities.minImageExtent.height,
							  min(deviceSurfaceProperties.surfaceCapabilities.maxImageExtent.height,
								  m_Extent.height));
	}

	uint32_t imageCount = deviceSurfaceProperties.surfaceCapabilities.minImageCount + 1;
	if (deviceSurfaceProperties.surfaceCapabilities.maxImageCount > 0 &&
		imageCount > deviceSurfaceProperties.surfaceCapabilities.maxImageCount)
	{ imageCount = deviceSurfaceProperties.surfaceCapabilities.maxImageCount; }
	vk::SwapchainCreateInfoKHR swapChainCreateInfo(
		{}, m_Surface.get(), imageCount, m_SwapChainImageFormat, surfaceFormat.colorSpace, m_Extent,
		1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr,
		deviceSurfaceProperties.surfaceCapabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, nullptr);

	if (physicalDevice.GetPresentQueueFamily().m_Index !=
		physicalDevice.GetGraphicsQueueFamily().m_Index)
	{
		uint32_t queueFamilyIndices[] = {physicalDevice.GetGraphicsQueueFamily().m_Index,
										 physicalDevice.GetPresentQueueFamily().m_Index};
		swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	m_SwapChain.reset();
	m_SwapChain = logicalDevice.GetHandle().createSwapchainKHRUnique(swapChainCreateInfo);
	m_SwapChainImages = logicalDevice.GetHandle().getSwapchainImagesKHR(m_SwapChain.get());
}

void Neon::VulkanRenderer::RecreateSwapChain()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_Window->GetNativeWindow(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_Window->GetNativeWindow(), &width, &height);
		glfwWaitEvents();
	}

	device.waitIdle();
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateOffscreenRenderer();
	CreateImGuiRenderer();
	CreateCommandBuffers();
	CreateSwapChainDependencies();
	device.waitIdle();
}

void Neon::VulkanRenderer::IntegrateImGui()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice();
	const auto& physicalDevice = Neon::Context::GetInstance().GetPhysicalDevice();
	ImGui_ImplVulkan_Shutdown();
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		{vk::DescriptorType::eSampler, 1000},
		{vk::DescriptorType::eCombinedImageSampler, 1000},
		{vk::DescriptorType::eSampledImage, 1000},
		{vk::DescriptorType::eStorageImage, 1000},
		{vk::DescriptorType::eUniformTexelBuffer, 1000},
		{vk::DescriptorType::eStorageTexelBuffer, 1000},
		{vk::DescriptorType::eUniformBuffer, 1000},
		{vk::DescriptorType::eStorageBuffer, 1000},
		{vk::DescriptorType::eUniformBufferDynamic, 1000},
		{vk::DescriptorType::eStorageBufferDynamic, 1000},
		{vk::DescriptorType::eInputAttachment, 1000}};

	m_ImGuiDescriptorPool = vk::UniqueDescriptorPool(
		Neon::CreateDescriptorPool(device.GetHandle(), poolSizes,
								   1000 * static_cast<uint32_t>(poolSizes.size())),
		device.GetHandle());

	ImGui_ImplVulkan_InitInfo imguiInfo = {};
	imguiInfo.Instance = Neon::Context::GetInstance().GetVkInstance();
	imguiInfo.PhysicalDevice = physicalDevice.GetHandle();
	imguiInfo.Device = device.GetHandle();
	imguiInfo.QueueFamily = physicalDevice.GetGraphicsQueueFamily().m_Index;
	imguiInfo.Queue = device.GetGraphicsQueue();
	imguiInfo.PipelineCache = nullptr;
	imguiInfo.DescriptorPool = m_ImGuiDescriptorPool.get();
	imguiInfo.Allocator = nullptr;
	imguiInfo.MinImageCount = (uint32_t)m_SwapChainImages.size();
	imguiInfo.ImageCount = (uint32_t)m_SwapChainImages.size();
	imguiInfo.CheckVkResultFn = nullptr;
	ImGui_ImplVulkan_Init(&imguiInfo, m_ImGuiRenderPass.get());

	vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	EndSingleTimeCommands(commandBuffer);

	m_ImGuiOffscreenTextureDescSet = ImGui_ImplVulkan_CreateTexture();
	ImGui_ImplVulkan_UpdateTexture(
		m_ImGuiOffscreenTextureDescSet, m_OffscreenImageAllocation.descriptor.sampler,
		m_OffscreenImageAllocation.descriptor.imageView,
		(VkImageLayout)m_OffscreenImageAllocation.descriptor.imageLayout);
}

void Neon::VulkanRenderer::CreateSwapChainImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size());
	for (uint32_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		m_SwapChainImageViews[i] = CreateImageViewUnique(
			m_SwapChainImages[i], m_SwapChainImageFormat, vk::ImageAspectFlagBits::eColor);
	}
}

void Neon::VulkanRenderer::CreateOffscreenRenderer()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	m_SampledImage.textureAllocation = Neon::Allocator::CreateImage(
		m_Extent.width, m_Extent.height, msaaSamples, vk::Format::eR32G32B32A32Sfloat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(m_SampledImage.textureAllocation.image,
										   vk::ImageAspectFlagBits::eColor,
										   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	m_SampledImage.descriptor.imageView =
		CreateImageView(m_SampledImage.textureAllocation.image, vk::Format::eR32G32B32A32Sfloat,
						vk::ImageAspectFlagBits::eColor);

	m_OffscreenImageAllocation.textureAllocation = Neon::Allocator::CreateImage(
		m_Extent.width, m_Extent.height, vk::SampleCountFlagBits::e1,
		vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
			vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(m_OffscreenImageAllocation.textureAllocation.image,
										   vk::ImageAspectFlagBits::eColor,
										   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	m_OffscreenImageAllocation.descriptor.imageView =
		CreateImageView(m_OffscreenImageAllocation.textureAllocation.image,
						vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
	m_OffscreenImageAllocation.descriptor.sampler = CreateSampler(vk::SamplerCreateInfo());
	m_OffscreenImageAllocation.descriptor.imageLayout = vk::ImageLayout::eGeneral;

	m_OffscreenDepthImageAllocation.textureAllocation = Neon::Allocator::CreateImage(
		m_Extent.width, m_Extent.height, msaaSamples, vk::Format::eD32Sfloat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(
		m_OffscreenDepthImageAllocation.textureAllocation.image, vk::ImageAspectFlagBits::eDepth,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	m_OffscreenDepthImageAllocation.descriptor.imageView =
		CreateImageView(m_OffscreenDepthImageAllocation.textureAllocation.image,
						vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth);
	m_OffscreenRenderPass = vk::UniqueRenderPass(
		Neon::CreateRenderPass(device, vk::Format::eR32G32B32A32Sfloat, msaaSamples, true,
							   vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral,
							   vk::Format::eD32Sfloat, true,
							   vk::ImageLayout::eDepthStencilAttachmentOptimal,
							   vk::ImageLayout::eDepthStencilAttachmentOptimal, true),
		device);

	m_OffscreenFrameBuffers.resize(m_SwapChainImageViews.size());
	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
	{
		std::vector<vk::ImageView> attachments = {
			m_SampledImage.descriptor.imageView,
			m_OffscreenDepthImageAllocation.descriptor.imageView,
			m_OffscreenImageAllocation.descriptor.imageView};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.setRenderPass(m_OffscreenRenderPass.get());
		framebufferInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
		framebufferInfo.setPAttachments(attachments.data());
		framebufferInfo.setWidth(m_Extent.width);
		framebufferInfo.setHeight(m_Extent.height);
		framebufferInfo.setLayers(1);

		m_OffscreenFrameBuffers[i] = device.createFramebufferUnique(framebufferInfo);
	}
}

void Neon::VulkanRenderer::CreateImGuiRenderer()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	m_ImGuiRenderPass = vk::UniqueRenderPass(
		Neon::CreateRenderPass(device, m_SwapChainImageFormat, vk::SampleCountFlagBits::e1, false,
							   vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR),
		device);

	m_ImGuiFrameBuffers.resize(m_SwapChainImageViews.size());
	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
	{
		std::array<vk::ImageView, 1> attachments = {m_SwapChainImageViews[i].get()};
		vk::FramebufferCreateInfo framebufferInfo{{},
												  m_ImGuiRenderPass.get(),
												  static_cast<uint32_t>(attachments.size()),
												  attachments.data(),
												  m_Extent.width,
												  m_Extent.height,
												  1};
		m_ImGuiFrameBuffers[i] = device.createFramebufferUnique(framebufferInfo);
	}
}

void Neon::VulkanRenderer::CreateOffscreenGraphicsPipeline()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1,
						  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(bindings.size()),
												 bindings.data());
	m_WavefrontLayout = device.createDescriptorSetLayout(layoutInfo);
	m_OffscreenGraphicsPipeline.Init(device);
	m_OffscreenGraphicsPipeline.LoadVertexShader("src/Shaders/vert.spv");
	m_OffscreenGraphicsPipeline.LoadFragmentShader("src/Shaders/frag.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
												   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	m_OffscreenGraphicsPipeline.CreatePipelineLayout({m_WavefrontLayout}, {pushConstantRange});
	m_OffscreenGraphicsPipeline.CreatePipeline(
		m_OffscreenRenderPass.get(), msaaSamples, m_Extent, {Vertex::getBindingDescription()},
		{Vertex::getAttributeDescriptions()}, vk::CullModeFlagBits::eBack);
}

void Neon::VulkanRenderer::CreateCommandPool()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	const auto& physicalDevice = Neon::Context::GetInstance().GetPhysicalDevice();
	vk::CommandPoolCreateInfo poolInfo{vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
									   physicalDevice.GetGraphicsQueueFamily().m_Index};
	m_CommandPool = device.createCommandPoolUnique(poolInfo);
}

void Neon::VulkanRenderer::CreateUniformBuffers(
	std::vector<Neon::BufferAllocation>& bufferAllocations, vk::DeviceSize bufferSize)
{
	bufferAllocations.resize(m_SwapChainImages.size());
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		bufferAllocations[i] = Neon::Allocator::CreateBuffer(
			bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void Neon::VulkanRenderer::CreateCommandBuffers()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	m_CommandBuffers.resize(m_SwapChainImages.size());
	vk::CommandBufferAllocateInfo allocInfo{m_CommandPool.get(), vk::CommandBufferLevel::ePrimary,
											static_cast<uint32_t>(m_CommandBuffers.size())};
	m_CommandBuffers = device.allocateCommandBuffersUnique(allocInfo);
}

void Neon::VulkanRenderer::CreateSyncObjects()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	m_ImagesInFlight.resize(m_SwapChainImages.size());

	vk::SemaphoreCreateInfo semaphoreInfo{};
	vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ImageAvailableSemaphores.push_back(device.createSemaphoreUnique(semaphoreInfo));
		m_RenderFinishedSemaphores.push_back(device.createSemaphoreUnique(semaphoreInfo));
		m_InFlightFences.push_back(device.createFenceUnique(fenceInfo));
	}
}

void Neon::VulkanRenderer::UpdateCameraMatrices(const Neon::PerspectiveCamera& camera)
{
	auto view = camera.GetViewMatrix();
	auto projection = camera.GetProjectionMatrix();
	CameraMatrices m{camera.GetPosition(), view, projection};
	Neon::Allocator::UpdateAllocation(m_CameraBufferAllocations[s_ImageIndex].allocation, m);
}

vk::Extent2D Neon::VulkanRenderer::GetExtent2D()
{
	return s_Instance.m_Extent;
}

void* Neon::VulkanRenderer::GetOffscreenImageID()
{
	return s_Instance.m_ImGuiOffscreenTextureDescSet;
}

void Neon::VulkanRenderer::CreateWavefrontDescriptorSet(MaterialComponent& materialComponent)
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1,
						  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);

	if (s_Instance.m_DescriptorPools.empty())
	{
		s_Instance.m_DescriptorPools.emplace_back(
			Neon::CreateDescriptorPool(device, bindings,
									   MAX_SWAP_CHAIN_IMAGES * MAX_DESCRIPTOR_SETS_PER_POOL),
			device);
	}

	vk::DescriptorBufferInfo cameraBufferInfo{s_Instance.m_CameraBufferAllocations[0].buffer, 0,
											  sizeof(CameraMatrices)};
	vk::DescriptorBufferInfo materialBufferInfo{materialComponent.m_MaterialBuffer.buffer, 0,
												VK_WHOLE_SIZE};

	std::vector<vk::DescriptorImageInfo> texturesBufferInfo;
	texturesBufferInfo.reserve(materialComponent.m_TextureImages.size());
	for (auto& textureImage : materialComponent.m_TextureImages)
	{
		texturesBufferInfo.push_back(textureImage.descriptor);
	}

	materialComponent.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = materialComponent.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.CreateDescriptorSet(
			s_Instance.m_DescriptorPools[s_Instance.m_DescriptorPools.size() - 1].get(), bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &cameraBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(1, &materialBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(2, texturesBufferInfo.data(), 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}
}
