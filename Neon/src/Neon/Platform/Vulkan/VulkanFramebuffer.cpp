#include "neopch.h"

#include "VulkanContext.h"
#include "VulkanFrameBuffer.h"
#include "VulkanRenderPass.h"

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

namespace Neon
{

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: Framebuffer(spec)
	{
		m_VulkanRenderPass = m_Specification.Pass.As<VulkanRenderPass>()->GetHandle();

		m_ColorImageId = ImGui_ImplVulkan_CreateTexture();

		Resize(spec.Width, spec.Height, true);
	}

	void VulkanFramebuffer::Resize(uint32 width, uint32 height, bool forceRecreate /*= false*/)
	{
		if (width == m_Specification.Width && height == m_Specification.Height && !forceRecreate)
		{
			return;
		}
		m_Specification.Width = width;
		m_Specification.Height = height;
		if (!m_Specification.SwapChainTarget)
		{
			const auto& device = VulkanContext::GetDevice();

			VulkanAllocator allocator(device, "Framebuffer");

			std::vector<vk::ImageView> attachments;

			// COLOR ATTACHMENT
			if (m_Specification.Pass->GetSpecification().HasColor)
			{
				const vk::Format COLOR_BUFFER_FORMAT =
					FramebufferFormatToVulkanFormat(m_Specification.Pass->GetSpecification().ColorFormat);

				vk::ImageCreateInfo imageCreateInfo = {};
				imageCreateInfo.imageType = vk::ImageType::e2D;
				imageCreateInfo.format = COLOR_BUFFER_FORMAT;
				imageCreateInfo.extent.width = width;
				imageCreateInfo.extent.height = height;
				imageCreateInfo.extent.depth = 1;
				imageCreateInfo.mipLevels = 1;
				imageCreateInfo.arrayLayers = 1;
				imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
				imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
				// We will sample directly from the color attachment
				imageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;

				m_ColorAttachment.Image = device->GetHandle().createImageUnique(imageCreateInfo);

				vk::MemoryRequirements memReqs = device->GetHandle().getImageMemoryRequirements(m_ColorAttachment.Image.get());
				allocator.Allocate(memReqs, m_ColorAttachment.Memory);

				device->GetHandle().bindImageMemory(m_ColorAttachment.Image.get(), m_ColorAttachment.Memory.get(), 0);

				vk::ImageViewCreateInfo colorImageViewCreateInfo = {};
				colorImageViewCreateInfo.viewType = vk::ImageViewType::e2D;
				colorImageViewCreateInfo.format = COLOR_BUFFER_FORMAT;
				colorImageViewCreateInfo.subresourceRange = {};
				colorImageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				colorImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
				colorImageViewCreateInfo.subresourceRange.levelCount = 1;
				colorImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				colorImageViewCreateInfo.subresourceRange.layerCount = 1;
				colorImageViewCreateInfo.image = m_ColorAttachment.Image.get();
				m_ColorAttachment.View = device->GetHandle().createImageViewUnique(colorImageViewCreateInfo);

				// Create sampler to sample from the attachment in the fragment shader
				vk::SamplerCreateInfo samplerCreateInfo = {};
				samplerCreateInfo.maxAnisotropy = 1.0f;
				samplerCreateInfo.magFilter = vk::Filter::eLinear;
				samplerCreateInfo.minFilter = vk::Filter::eLinear;
				samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
				samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
				samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
				samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
				samplerCreateInfo.mipLodBias = 0.0f;
				samplerCreateInfo.maxAnisotropy = 1.0f;
				samplerCreateInfo.minLod = 0.0f;
				samplerCreateInfo.maxLod = 1.0f;
				samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
				m_ColorAttachmentSampler = device->GetHandle().createSamplerUnique(samplerCreateInfo);

				attachments.push_back(m_ColorAttachment.View.get());
			}

			// DEPTH ATTACHMENT
			if (m_Specification.Pass->GetSpecification().HasDepth)
			{
				vk::Format depthFormat = VulkanContext::GetDevice()->GetPhysicalDevice()->GetDepthFormat();

				vk::ImageCreateInfo imageCreateInfo = {};
				imageCreateInfo.imageType = vk::ImageType::e2D;
				imageCreateInfo.format = depthFormat;
				imageCreateInfo.extent.width = width;
				imageCreateInfo.extent.height = height;
				imageCreateInfo.extent.depth = 1;
				imageCreateInfo.mipLevels = 1;
				imageCreateInfo.arrayLayers = 1;
				imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
				imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
				// We will sample directly from the color attachment
				imageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;

				m_DepthAttachment.Image = device->GetHandle().createImageUnique(imageCreateInfo);
				vk::MemoryRequirements memReqs = device->GetHandle().getImageMemoryRequirements(m_DepthAttachment.Image.get());
				allocator.Allocate(memReqs, m_DepthAttachment.Memory);

				device->GetHandle().bindImageMemory(m_DepthAttachment.Image.get(), m_DepthAttachment.Memory.get(), 0);

				vk::ImageViewCreateInfo depthStencilImageViewCreateInfo = {};
				depthStencilImageViewCreateInfo.viewType = vk::ImageViewType::e2D;
				depthStencilImageViewCreateInfo.format = depthFormat;
				depthStencilImageViewCreateInfo.subresourceRange = {};
				depthStencilImageViewCreateInfo.subresourceRange.aspectMask =
					vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
				depthStencilImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
				depthStencilImageViewCreateInfo.subresourceRange.levelCount = 1;
				depthStencilImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				depthStencilImageViewCreateInfo.subresourceRange.layerCount = 1;
				depthStencilImageViewCreateInfo.image = m_DepthAttachment.Image.get();
				m_DepthAttachment.View = device->GetHandle().createImageViewUnique(depthStencilImageViewCreateInfo);

				attachments.push_back(m_DepthAttachment.View.get());
			}

			vk::FramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.renderPass = m_VulkanRenderPass;
			framebufferCreateInfo.attachmentCount = static_cast<uint32>(attachments.size());
			framebufferCreateInfo.pAttachments = attachments.data();
			framebufferCreateInfo.width = width;
			framebufferCreateInfo.height = height;
			framebufferCreateInfo.layers = 1;

			m_Handle = device->GetHandle().createFramebufferUnique(framebufferCreateInfo);

			// Fill a descriptor for later use in a descriptor set
			m_DescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			m_DescriptorImageInfo.imageView = m_ColorAttachment.View.get();
			m_DescriptorImageInfo.sampler = m_ColorAttachmentSampler.get();

			ImGui_ImplVulkan_UpdateTexture(m_ColorImageId, m_ColorAttachmentSampler.get(), m_ColorAttachment.View.get(),
										   (VkImageLayout)m_DescriptorImageInfo.imageLayout);
		}
		else
		{
			const VulkanSwapChain& swapChain = VulkanContext::Get()->GetSwapChain();
			m_VulkanRenderPass = swapChain.GetRenderPass();
		}
	}
} // namespace Neon
