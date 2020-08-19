#include "VulkanRenderer.h"

#include "Allocator.h"
#include "Context.h"
#include "RenderPass.h"
#include "Window.h"

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

#define MAX_SWAP_CHAIN_IMAGES 8

#define MAX_DESCRIPTOR_SETS_PER_POOL 1024

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

auto msaaSamples = vk::SampleCountFlagBits::e8;

Neon::VulkanRenderer Neon::VulkanRenderer::s_Instance;

static std::vector<const char*> instanceExtensions;

Neon::VulkanRenderer::VulkanRenderer() noexcept { }

void Neon::VulkanRenderer::Init(Window* window)
{
	instanceExtensions.push_back({VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME});
	s_Instance.InitRenderer(window);
}

void Neon::VulkanRenderer::Shutdown()
{
	Neon::Context::GetInstance().GetLogicalDevice().GetHandle().waitIdle();
}

void Neon::VulkanRenderer::Begin()
{
	auto result = s_Instance.m_SwapChain->AcquireNextImage();
	if (result == vk::Result::eErrorOutOfDateKHR) { s_Instance.WindowResized(); }
	else
	{
		assert(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);
		vk::CommandBufferBeginInfo beginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
		s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get().begin(beginInfo);
	}
}

void Neon::VulkanRenderer::End()
{
	auto& commandBuffer =
		s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get();
	commandBuffer.end();
	auto result = s_Instance.m_SwapChain->Present(commandBuffer);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{ s_Instance.WindowResized(); }
	else
	{
		assert(result == vk::Result::eSuccess);
	}
}

void Neon::VulkanRenderer::BeginScene(const Neon::PerspectiveCamera& camera,
									  const glm::vec4& clearColor)
{
	auto& commandBuffer =
		s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get();
	s_Instance.UpdateCameraMatrices(camera);
	std::array<vk::ClearValue, 3> clearValues = {};
	memcpy(&clearValues[0].color.float32, &clearColor, sizeof(clearValues[0].color.float32));
	clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
	memcpy(&clearValues[2].color.float32, &clearColor, sizeof(clearValues[0].color.float32));
	vk::RenderPassBeginInfo renderPassInfo{
		s_Instance.m_OffscreenRenderPass.get(),
		s_Instance.m_OffscreenFrameBuffers[s_Instance.m_SwapChain->GetImageIndex()].get(),
		{{0, 0}, s_Instance.m_SwapChain->GetExtent()},
		static_cast<uint32_t>(clearValues.size()),
		clearValues.data()};
	commandBuffer.beginRenderPass(
		renderPassInfo, vk::SubpassContents::eInline);
}

void Neon::VulkanRenderer::EndScene()
{
	s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get().endRenderPass();
}

void Neon::VulkanRenderer::DrawImGui()
{
	auto& commandBuffer =
		s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get();

	vk::RenderPassBeginInfo renderPassInfo{
		s_Instance.m_ImGuiRenderPass.get(),
		s_Instance.m_ImGuiFrameBuffers[s_Instance.m_SwapChain->GetImageIndex()].get(),
		{{0, 0}, s_Instance.m_SwapChain->GetExtent()}};
	commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	ImGui_ImplVulkan_RenderDrawData(
		ImGui::GetDrawData(),
		s_Instance.m_CommandBuffers[s_Instance.m_SwapChain->GetImageIndex()].get());
	commandBuffer.endRenderPass();
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
	Neon::Context::GetInstance().Init();
	Neon::Context::GetInstance().CreateSurface(window);
	Neon::Context::GetInstance().CreateDevice({vk::QueueFlagBits::eGraphics});
	Neon::Context::GetInstance().InitAllocator();

	const auto& physicalDevice = Neon::Context::GetInstance().GetPhysicalDevice();
	const auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice();
	m_SwapChain =
		SwapChain::Create(*window, Context::GetInstance().GetVkInstance(),
						  Context::GetInstance().GetSurface(), physicalDevice, logicalDevice);

	m_OffscreenRenderPass = vk::UniqueRenderPass(
		Neon::CreateRenderPass(logicalDevice.GetHandle(), vk::Format::eR32G32B32A32Sfloat,
							   msaaSamples, true, vk::ImageLayout::eGeneral,
							   vk::ImageLayout::eGeneral, vk::Format::eD32Sfloat, true,
							   vk::ImageLayout::eDepthStencilAttachmentOptimal,
							   vk::ImageLayout::eDepthStencilAttachmentOptimal, true),
		logicalDevice.GetHandle());

	CreateCommandPool();
	IntegrateImGui();
	CreateOffscreenRenderer();
	CreateImGuiRenderer();
	CreateCommandBuffers();
	CreateUniformBuffers(m_CameraBufferAllocations, sizeof(CameraMatrices));

	///////////////////////////
	std::vector<vk::DescriptorPoolSize> sizes;
	sizes.emplace_back(vk::DescriptorType::eUniformBuffer,
					   1 * MAX_SWAP_CHAIN_IMAGES * MAX_DESCRIPTOR_SETS_PER_POOL);
	sizes.emplace_back(vk::DescriptorType::eStorageBuffer,
					   1 * MAX_SWAP_CHAIN_IMAGES * MAX_DESCRIPTOR_SETS_PER_POOL);
	sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler,
					   10 * MAX_SWAP_CHAIN_IMAGES * MAX_DESCRIPTOR_SETS_PER_POOL);
	m_DescriptorPools.push_back(DescriptorPool::Create(
		logicalDevice.GetHandle(), sizes, MAX_SWAP_CHAIN_IMAGES * MAX_DESCRIPTOR_SETS_PER_POOL));
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1,
						  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	m_CameraDescriptorSets.resize(m_SwapChain->GetImageViewSize());
	for (int i = 0; i < m_SwapChain->GetImageViewSize(); i++)
	{
		vk::DescriptorBufferInfo cameraBufferInfo{m_CameraBufferAllocations[i]->buffer, 0,
												  sizeof(CameraMatrices)};
		auto& cameraDescriptorSet = m_CameraDescriptorSets[i];
		cameraDescriptorSet.Init(logicalDevice.GetHandle());
		cameraDescriptorSet.Create(
			s_Instance.m_DescriptorPools[s_Instance.m_DescriptorPools.size() - 1]->GetHandle(),
			bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			cameraDescriptorSet.CreateWrite(0, &cameraBufferInfo, 0)};
		cameraDescriptorSet.Update(descriptorWrites);
	}
	////////////////////////

	Neon::Context::GetInstance().GetLogicalDevice().GetHandle().waitIdle();
}

void Neon::VulkanRenderer::WindowResized()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	device.waitIdle();
	CreateOffscreenRenderer();
	CreateImGuiRenderer();
	device.waitIdle();
}

void Neon::VulkanRenderer::IntegrateImGui()
{
	const auto& logicalDevice = Neon::Context::GetInstance().GetLogicalDevice();
	const auto& physicalDevice = Neon::Context::GetInstance().GetPhysicalDevice();

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
	m_ImGuiDescriptorPool = DescriptorPool::Create(logicalDevice.GetHandle(), poolSizes,
												   1000 * static_cast<uint32_t>(poolSizes.size()));

	m_ImGuiRenderPass = vk::UniqueRenderPass(
		Neon::CreateRenderPass(logicalDevice.GetHandle(), m_SwapChain->GetSwapChainImageFormat(),
							   vk::SampleCountFlagBits::e1, false, vk::ImageLayout::eUndefined,
							   vk::ImageLayout::ePresentSrcKHR),
		logicalDevice.GetHandle());

	ImGui_ImplVulkan_InitInfo imguiInfo = {};
	imguiInfo.Instance = Neon::Context::GetInstance().GetVkInstance();
	imguiInfo.PhysicalDevice = physicalDevice.GetHandle();
	imguiInfo.Device = logicalDevice.GetHandle();
	imguiInfo.QueueFamily = physicalDevice.GetGraphicsQueueFamily().m_Index;
	imguiInfo.Queue = logicalDevice.GetGraphicsQueue();
	imguiInfo.PipelineCache = nullptr;
	imguiInfo.DescriptorPool = m_ImGuiDescriptorPool->GetHandle();
	imguiInfo.Allocator = nullptr;
	imguiInfo.MinImageCount = (uint32_t)m_SwapChain->GetImageViewSize();
	imguiInfo.ImageCount = (uint32_t)m_SwapChain->GetImageViewSize();
	imguiInfo.CheckVkResultFn = nullptr;
	ImGui_ImplVulkan_Init(&imguiInfo, m_ImGuiRenderPass.get());

	vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	EndSingleTimeCommands(commandBuffer);

	m_ImGuiOffscreenTextureDescSet = ImGui_ImplVulkan_CreateTexture();
}

void Neon::VulkanRenderer::CreateOffscreenRenderer()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	const auto& extent = s_Instance.m_SwapChain->GetExtent();
	m_SampledImage.textureAllocation = Allocator::CreateImage(
		extent.width, extent.height, msaaSamples, vk::Format::eR32G32B32A32Sfloat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(m_SampledImage.textureAllocation->image,
										   vk::ImageAspectFlagBits::eColor,
										   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	m_SampledImage.descriptor.imageView =
		CreateImageView(m_SampledImage.textureAllocation->image, vk::Format::eR32G32B32A32Sfloat,
						vk::ImageAspectFlagBits::eColor);

	m_OffscreenDepthImageAllocation.textureAllocation = Neon::Allocator::CreateImage(
		extent.width, extent.height, msaaSamples, vk::Format::eD32Sfloat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(
		m_OffscreenDepthImageAllocation.textureAllocation->image, vk::ImageAspectFlagBits::eDepth,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	m_OffscreenDepthImageAllocation.descriptor.imageView =
		CreateImageView(m_OffscreenDepthImageAllocation.textureAllocation->image,
						vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth);

	m_OffscreenImageAllocation.textureAllocation = Neon::Allocator::CreateImage(
		extent.width, extent.height, vk::SampleCountFlagBits::e1, vk::Format::eR32G32B32A32Sfloat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
			vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(m_OffscreenImageAllocation.textureAllocation->image,
										   vk::ImageAspectFlagBits::eColor,
										   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	m_OffscreenImageAllocation.descriptor.imageView =
		CreateImageView(m_OffscreenImageAllocation.textureAllocation->image,
						vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
	m_OffscreenImageAllocation.descriptor.sampler = CreateSampler(vk::SamplerCreateInfo());
	m_OffscreenImageAllocation.descriptor.imageLayout = vk::ImageLayout::eGeneral;

	m_OffscreenFrameBuffers.clear();
	m_OffscreenFrameBuffers.reserve(m_SwapChain->GetImageViewSize());
	for (size_t i = 0; i < m_SwapChain->GetImageViewSize(); i++)
	{
		std::vector<vk::ImageView> attachments = {
			m_SampledImage.descriptor.imageView,
			m_OffscreenDepthImageAllocation.descriptor.imageView,
			m_OffscreenImageAllocation.descriptor.imageView};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.setRenderPass(m_OffscreenRenderPass.get());
		framebufferInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
		framebufferInfo.setPAttachments(attachments.data());
		framebufferInfo.setWidth(extent.width);
		framebufferInfo.setHeight(extent.height);
		framebufferInfo.setLayers(1);

		m_OffscreenFrameBuffers.push_back(device.createFramebufferUnique(framebufferInfo));
	}
	ImGui_ImplVulkan_UpdateTexture(
		m_ImGuiOffscreenTextureDescSet, m_OffscreenImageAllocation.descriptor.sampler,
		m_OffscreenImageAllocation.descriptor.imageView,
		(VkImageLayout)m_OffscreenImageAllocation.descriptor.imageLayout);
}

void Neon::VulkanRenderer::CreateImGuiRenderer()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	m_ImGuiFrameBuffers.clear();
	m_ImGuiFrameBuffers.reserve(m_SwapChain->GetImageViewSize());
	const auto& extent = s_Instance.m_SwapChain->GetExtent();
	for (size_t i = 0; i < m_SwapChain->GetImageViewSize(); i++)
	{
		std::array<vk::ImageView, 1> attachments = {m_SwapChain->GetImageView(i)};
		vk::FramebufferCreateInfo framebufferInfo{{},
												  m_ImGuiRenderPass.get(),
												  static_cast<uint32_t>(attachments.size()),
												  attachments.data(),
												  extent.width,
												  extent.height,
												  1};
		m_ImGuiFrameBuffers.push_back(device.createFramebufferUnique(framebufferInfo));
	}
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
	std::vector<std::unique_ptr<BufferAllocation>>& bufferAllocations, vk::DeviceSize bufferSize)
{
	bufferAllocations.resize(m_SwapChain->GetImageViewSize());
	for (size_t i = 0; i < m_SwapChain->GetImageViewSize(); i++)
	{
		bufferAllocations[i] = Allocator::CreateBuffer(
			bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void Neon::VulkanRenderer::CreateCommandBuffers()
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();
	m_CommandBuffers.resize(m_SwapChain->GetImageViewSize());
	vk::CommandBufferAllocateInfo allocInfo{m_CommandPool.get(), vk::CommandBufferLevel::ePrimary,
											static_cast<uint32_t>(m_CommandBuffers.size())};
	m_CommandBuffers = device.allocateCommandBuffersUnique(allocInfo);
}

void Neon::VulkanRenderer::UpdateCameraMatrices(const Neon::PerspectiveCamera& camera)
{
	auto view = camera.GetViewMatrix();
	auto projection = camera.GetProjectionMatrix();
	CameraMatrices m{camera.GetPosition(), view, projection};
	Neon::Allocator::UpdateAllocation(
		m_CameraBufferAllocations[m_SwapChain->GetImageIndex()]->allocation, m);
}

vk::Extent2D Neon::VulkanRenderer::GetExtent2D()
{
	return s_Instance.m_SwapChain->GetExtent();
}

void* Neon::VulkanRenderer::GetOffscreenImageID()
{
	return s_Instance.m_ImGuiOffscreenTextureDescSet;
}

void Neon::VulkanRenderer::LoadSkyDome(Neon::SkyDomeRenderer& skyDomeRenderer)
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;

	auto& pipeline = skyDomeRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/vert_skydome.spv");
	pipeline.LoadFragmentShader("src/Shaders/frag_skydome.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
											   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({s_Instance.m_CameraDescriptorSets[0].GetLayout()},
								  {pushConstantRange});
	pipeline.CreatePipeline(s_Instance.m_OffscreenRenderPass.get(), msaaSamples,
							s_Instance.m_SwapChain->GetExtent(), {Vertex::getBindingDescription()},
							{Vertex::getAttributeDescriptions()}, vk::CullModeFlagBits::eFront);
}

void Neon::VulkanRenderer::LoadModel(Neon::MeshRenderer& meshRenderer)
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler,
						  static_cast<uint32_t>(meshRenderer.m_TextureImages.size()),
						  vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorBufferInfo materialBufferInfo{meshRenderer.m_MaterialBuffer->buffer, 0,
												VK_WHOLE_SIZE};

	std::vector<vk::DescriptorImageInfo> texturesBufferInfo;
	texturesBufferInfo.reserve(meshRenderer.m_TextureImages.size());
	for (auto& textureImage : meshRenderer.m_TextureImages)
	{
		texturesBufferInfo.push_back(textureImage->descriptor);
	}

	meshRenderer.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = meshRenderer.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.Create(
			s_Instance.m_DescriptorPools[s_Instance.m_DescriptorPools.size() - 1]->GetHandle(),
			bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &materialBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(1, texturesBufferInfo.data(), 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}
	auto& pipeline = meshRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/vert.spv");
	pipeline.LoadFragmentShader("src/Shaders/frag.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
											   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({s_Instance.m_CameraDescriptorSets[0].GetLayout(),
								   meshRenderer.m_DescriptorSets[0].GetLayout()},
								  {pushConstantRange});
	pipeline.CreatePipeline(s_Instance.m_OffscreenRenderPass.get(), msaaSamples,
							s_Instance.m_SwapChain->GetExtent(), {Vertex::getBindingDescription()},
							{Vertex::getAttributeDescriptions()}, vk::CullModeFlagBits::eBack);
}

void Neon::VulkanRenderer::LoadAnimatedModel(Neon::SkinnedMeshRenderer& skinnedMeshRenderer)
{
	assert(skinnedMeshRenderer.m_BoneBuffer);
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler,
						  static_cast<uint32_t>(skinnedMeshRenderer.m_TextureImages.size()),
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(2, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eVertex);

	vk::DescriptorBufferInfo materialBufferInfo{skinnedMeshRenderer.m_MaterialBuffer->buffer, 0,
												VK_WHOLE_SIZE};

	std::vector<vk::DescriptorImageInfo> texturesBufferInfo;
	texturesBufferInfo.reserve(skinnedMeshRenderer.m_TextureImages.size());
	for (auto& textureImage : skinnedMeshRenderer.m_TextureImages)
	{
		texturesBufferInfo.push_back(textureImage->descriptor);
	}

	skinnedMeshRenderer.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	vk::DescriptorBufferInfo boneBufferInfo{skinnedMeshRenderer.m_BoneBuffer->buffer, 0, VK_WHOLE_SIZE};
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = skinnedMeshRenderer.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.Create(
			s_Instance.m_DescriptorPools[s_Instance.m_DescriptorPools.size() - 1]->GetHandle(),
			bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &materialBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(1, texturesBufferInfo.data(), 0),
			wavefrontDescriptorSet.CreateWrite(2, &boneBufferInfo, 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}
	auto& pipeline = skinnedMeshRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/animation_vert.spv");
	pipeline.LoadFragmentShader("src/Shaders/frag.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
											   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({s_Instance.m_CameraDescriptorSets[0].GetLayout(),
								   skinnedMeshRenderer.m_DescriptorSets[0].GetLayout()},
								  {pushConstantRange});
	pipeline.CreatePipeline(s_Instance.m_OffscreenRenderPass.get(), msaaSamples,
							s_Instance.m_SwapChain->GetExtent(), {Vertex::getBindingDescription()},
							{Vertex::getAttributeDescriptions()}, vk::CullModeFlagBits::eBack);
}

void Neon::VulkanRenderer::LoadTerrain(MeshRenderer& meshRenderer)
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eCombinedImageSampler,
						  1,
						  vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorImageInfo texturesBufferInfo = meshRenderer.m_TextureImages[0]->descriptor;
	meshRenderer.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = meshRenderer.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.Create(
			s_Instance.m_DescriptorPools[s_Instance.m_DescriptorPools.size() - 1]->GetHandle(),
			bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &texturesBufferInfo, 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}

	auto& pipeline = meshRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/vert_terrain.spv");
	pipeline.LoadFragmentShader("src/Shaders/frag_terrain.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
											   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({s_Instance.m_CameraDescriptorSets[0].GetLayout(),
								   meshRenderer.m_DescriptorSets[0].GetLayout()},
								  {pushConstantRange});
	pipeline.CreatePipeline(s_Instance.m_OffscreenRenderPass.get(), msaaSamples,
							s_Instance.m_SwapChain->GetExtent(), {Vertex::getBindingDescription()},
							{Vertex::getAttributeDescriptions()}, vk::CullModeFlagBits::eBack);
}
