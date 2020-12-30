#pragma once

#include "Renderer/Framebuffer.h"
#include "Vulkan.h"

namespace Neon
{
	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);
		~VulkanFramebuffer() = default;
		void Resize(uint32 width, uint32 height, bool forceRecreate = false) override;

		vk::Framebuffer GetHandle() const
		{
			return m_Handle.get();
		}

		void* GetColorImageID() const override
		{
			return m_ColorImageId;
		}

	private:
		struct FrameBufferAttachment
		{
			vk::UniqueImage Image;
			vk::UniqueDeviceMemory Memory;
			vk::UniqueImageView View;
		};

		FrameBufferAttachment m_ColorAttachment;
		FrameBufferAttachment m_DepthAttachment;
		vk::RenderPass m_VulkanRenderPass;
		vk::UniqueSampler m_ColorAttachmentSampler;
		vk::UniqueFramebuffer m_Handle;
		vk::DescriptorImageInfo m_DescriptorImageInfo;

		VkDescriptorSet m_ColorImageId;
	};
} // namespace Neon
