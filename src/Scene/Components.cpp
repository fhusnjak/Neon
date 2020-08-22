//
// Created by Filip on 21.8.2020..
//

#include "Components.h"
#include <Renderer/Context.h>
#include <Renderer/VulkanRenderer.h>

Neon::WaterRenderer::WaterRenderer(vk::Extent2D extent)
{
	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	m_SampledImage.m_TextureAllocation = Allocator::CreateImage(
		extent.width, extent.height, VulkanRenderer::GetMsaaSamples(),
		vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(m_SampledImage.m_TextureAllocation->m_Image,
										   vk::ImageAspectFlagBits::eColor,
										   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	m_SampledImage.m_Descriptor.imageView = VulkanRenderer::CreateImageView(
		m_SampledImage.m_TextureAllocation->m_Image, vk::Format::eR32G32B32A32Sfloat,
		vk::ImageAspectFlagBits::eColor);

	m_DepthReflectionTexture.m_TextureAllocation = Neon::Allocator::CreateImage(
		extent.width, extent.height, VulkanRenderer::GetMsaaSamples(), vk::Format::eD32Sfloat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(
		m_DepthReflectionTexture.m_TextureAllocation->m_Image, vk::ImageAspectFlagBits::eDepth,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	m_DepthReflectionTexture.m_Descriptor.imageView =
		VulkanRenderer::CreateImageView(m_DepthReflectionTexture.m_TextureAllocation->m_Image,
										vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth);

	m_ReflectionTexture.m_TextureAllocation = Neon::Allocator::CreateImage(
		extent.width, extent.height, vk::SampleCountFlagBits::e1, vk::Format::eR32G32B32A32Sfloat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
			vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY);
	Neon::Allocator::TransitionImageLayout(m_ReflectionTexture.m_TextureAllocation->m_Image,
										   vk::ImageAspectFlagBits::eColor,
										   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	m_ReflectionTexture.m_Descriptor.imageView = VulkanRenderer::CreateImageView(
		m_ReflectionTexture.m_TextureAllocation->m_Image, vk::Format::eR32G32B32A32Sfloat,
		vk::ImageAspectFlagBits::eColor);
	m_ReflectionTexture.m_Descriptor.sampler =
		VulkanRenderer::CreateSampler(vk::SamplerCreateInfo());
	m_ReflectionTexture.m_Descriptor.imageLayout = vk::ImageLayout::eGeneral;

	m_FrameBuffers.clear();
	m_FrameBuffers.reserve(MAX_SWAP_CHAIN_IMAGES);
	for (size_t i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		std::vector<vk::ImageView> attachments = {m_SampledImage.m_Descriptor.imageView,
												  m_DepthReflectionTexture.m_Descriptor.imageView,
												  m_ReflectionTexture.m_Descriptor.imageView};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.setRenderPass(VulkanRenderer::GetOffscreenRenderPass());
		framebufferInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
		framebufferInfo.setPAttachments(attachments.data());
		framebufferInfo.setWidth(extent.width);
		framebufferInfo.setHeight(extent.height);
		framebufferInfo.setLayers(1);

		m_FrameBuffers.push_back(device.createFramebufferUnique(framebufferInfo));
	}

	m_TextureImages.push_back(std::move(m_ReflectionTexture));
}
