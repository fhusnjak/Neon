#include "neopch.h"

#include "VulkanContext.h"
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"

namespace Neon
{

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& specification)
		: m_Specification(specification)
	{
		vk::Device device = VulkanContext::GetDevice()->GetHandle();

		NEO_CORE_ASSERT(m_Specification.Shader, "Shader not initialized!");
		SharedRef<VulkanShader> vulkanShader = SharedRef<VulkanShader>(m_Specification.Shader);

		//NEO_CORE_ASSERT(m_Specification.RenderPass, "RenderPass not initialized!");
		//SharedRef<VulkanRenderPass> vulkanRenderPass = SharedRef<VulkanRenderPass>(m_Specification.RenderPass);

		vk::DescriptorSetLayout descriptorSetLayout = vulkanShader->GetDescriptorSetLayout();

		// TODO: Push constant ranges

		// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
		// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
		vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
		if (descriptorSetLayout != VK_NULL_HANDLE)
		{
			pPipelineLayoutCreateInfo.setLayoutCount = 1;
			pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		}

		m_PipelineLayout = device.createPipelineLayoutUnique(pPipelineLayoutCreateInfo);

		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {};
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipelineCreateInfo.layout = m_PipelineLayout.get();
		// Renderpass this pipeline is attached to
		// pipelineCreateInfo.renderPass = vulkanRenderPass->GetHandle();
		pipelineCreateInfo.renderPass = VulkanContext::Get()->GetSwapChain().GetRenderPass();

		// Construct the different states making up the pipeline

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		// Rasterization state
		vk::PipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.polygonMode = vk::PolygonMode::eFill;
		rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
		rasterizationState.frontFace = vk::FrontFace::eClockwise;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.lineWidth = 1.0f;

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used)
		vk::PipelineColorBlendAttachmentState blendAttachmentState[1] = {};
		blendAttachmentState[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
												 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		blendAttachmentState[0].blendEnable = VK_FALSE;
		vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = blendAttachmentState;

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overriden by the dynamic states (see below)
		vk::PipelineViewportStateCreateInfo viewportState = {};
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
		std::vector<vk::DynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(vk::DynamicState::eViewport);
		dynamicStateEnables.push_back(vk::DynamicState::eScissor);
		vk::PipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = static_cast<uint32>(dynamicStateEnables.size());

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.back.failOp = vk::StencilOp::eKeep;
		depthStencilState.back.passOp = vk::StencilOp::eKeep;
		//depthStencilState.back.compareOp = vk::CompareOp::eAlways;
		depthStencilState.front = depthStencilState.back;

		// Multi sampling state
		// This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		vk::PipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;

		// TODO: Vertex input descriptor

		// Vertex input state used for pipeline creation
		vk::PipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.vertexBindingDescriptionCount = 0;

		const auto& shaderStages = vulkanShader->GetShaderStages();
		// Set pipeline shader stage info
		pipelineCreateInfo.stageCount = static_cast<uint32>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		// Assign the pipeline states to the pipeline creation info structure
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;

		// TODO: Pipeline cache??
		/*
		vk::PipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		VkPipelineCache pipelineCache;
		device.createPipelineCache
		VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));*/

		// Create rendering pipeline using the specified states
		m_Handle = device.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo);

		// Shader modules are no longer needed once the graphics pipeline has been created
		// vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		// vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
	}

} // namespace Neon
