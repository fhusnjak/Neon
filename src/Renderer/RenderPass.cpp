#include "RenderPass.h"

vk::RenderPass Neon::CreateRenderPass(const vk::Device& device, vk::Format colorAttachmentFormat,
									  vk::SampleCountFlagBits samples, bool clearColor,
									  vk::ImageLayout colorInitialLayout,
									  vk::ImageLayout colorFinalLayout,
									  vk::Format depthAttachmentFormat, bool clearDepth,
									  vk::ImageLayout depthInitialLayout,
									  vk::ImageLayout depthFinalLayout, bool resolve)
{
	std::vector<vk::AttachmentDescription> attachments;
	vk::AttachmentDescription colorAttachment{{},
											  colorAttachmentFormat,
											  samples,
											  clearColor ? vk::AttachmentLoadOp::eClear
														 : vk::AttachmentLoadOp::eDontCare,
											  vk::AttachmentStoreOp::eStore,
											  vk::AttachmentLoadOp::eDontCare,
											  vk::AttachmentStoreOp::eDontCare,
											  colorInitialLayout,
											  colorFinalLayout};
	attachments.push_back(colorAttachment);
	vk::AttachmentReference colorAttachmentRef{0, vk::ImageLayout::eColorAttachmentOptimal};

	vk::SubpassDescription subpassDescription{
		{}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef};

	bool hasDepth = (depthAttachmentFormat != vk::Format::eUndefined);
	vk::AttachmentDescription depthAttachment;
	vk::AttachmentReference depthAttachmentRef;
	if (hasDepth)
	{
		depthAttachment = {{},
						   depthAttachmentFormat,
						   samples,
						   clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
						   vk::AttachmentStoreOp::eStore,
						   vk::AttachmentLoadOp::eDontCare,
						   vk::AttachmentStoreOp::eDontCare,
						   depthInitialLayout,
						   depthFinalLayout};
		attachments.push_back(depthAttachment);
		depthAttachmentRef = {1, vk::ImageLayout::eDepthStencilAttachmentOptimal};
		subpassDescription.setPDepthStencilAttachment(&depthAttachmentRef);
	}

	vk::SubpassDependency subpassDependency{VK_SUBPASS_EXTERNAL,
											0,
											vk::PipelineStageFlagBits::eColorAttachmentOutput,
											vk::PipelineStageFlagBits::eColorAttachmentOutput,
											vk::AccessFlagBits{},
											vk::AccessFlagBits::eColorAttachmentRead |
												vk::AccessFlagBits::eColorAttachmentWrite};

	std::vector<vk::AttachmentReference> resolveAttachments;
	if (resolve)
	{
		vk::AttachmentDescription colorAttachmentResolve = {{},
															colorAttachmentFormat,
															vk::SampleCountFlagBits::e1,
															vk::AttachmentLoadOp::eDontCare,
															vk::AttachmentStoreOp::eStore,
															vk::AttachmentLoadOp::eDontCare,
															vk::AttachmentStoreOp::eDontCare,
															colorInitialLayout,
															colorFinalLayout};
		vk::AttachmentDescription depthAttachmentResolve = {{},
															depthAttachmentFormat,
															vk::SampleCountFlagBits::e1,
															vk::AttachmentLoadOp::eDontCare,
															vk::AttachmentStoreOp::eStore,
															vk::AttachmentLoadOp::eDontCare,
															vk::AttachmentStoreOp::eDontCare,
															depthInitialLayout,
															depthFinalLayout};
		attachments.push_back(colorAttachmentResolve);
		attachments.push_back(depthAttachmentResolve);
		resolveAttachments.emplace_back(2, vk::ImageLayout::eColorAttachmentOptimal);
		resolveAttachments.emplace_back(3, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		subpassDescription.setPResolveAttachments(resolveAttachments.data());
	}

	vk::RenderPassCreateInfo renderPassInfo = {{},
											   static_cast<uint32_t>(attachments.size()),
											   attachments.data(),
											   1,
											   &subpassDescription,
											   1,
											   &subpassDependency};

	return device.createRenderPass(renderPassInfo);
}
