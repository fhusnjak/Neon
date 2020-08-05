#include "neopch.h"

#include "ObjModel.h"

#include "Core/Core.h"
#include "Tools/FileTools.h"

#include "VulkanRenderer.h"

std::vector<TextureImage> ObjModel::s_SkyboxTextureImages;
TextureImage ObjModel::s_Skysphere;
TextureImage ObjModel::s_HdrSkysphere;

static inline std::string GetPath(const std::string& file)
{
	std::string dir;
	size_t idx = file.find_last_of("\\/");
	if (idx != std::string::npos) dir = file.substr(0, idx);
	if (!dir.empty()) { dir += "\\"; }
	return dir;
}

void ObjModel::LoadSkysphere()
{
	std::string skysphereTexture = "birchwood.jpg";
	ImageAllocation imgAllocation = Allocator::CreateTextureImage("textures/" + skysphereTexture);
	vk::ImageView textureImageView = VulkanRenderer::CreateImageView(
		imgAllocation.image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	vk::SamplerCreateInfo samplerInfo = {
		{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = vk::CompareOp::eNever;

	samplerInfo.setMaxLod(FLT_MAX);
	vk::Sampler sampler = VulkanRenderer::CreateSampler(samplerInfo);
	vk::DescriptorImageInfo desc{sampler, textureImageView,
								 vk::ImageLayout::eShaderReadOnlyOptimal};
	s_Skysphere = {desc, imgAllocation};

	Allocator::FlushStaging();
}

void ObjModel::LoadHdrSkysphere()
{
	std::string skysphereTexture = "birchwood_16k.hdr";
	ImageAllocation imgAllocation =
		Allocator::CreateHdrTextureImage("textures/" + skysphereTexture);
	vk::ImageView textureImageView = VulkanRenderer::CreateImageView(
		imgAllocation.image, vk::Format::eR32G32B32Sfloat, vk::ImageAspectFlagBits::eColor);
	vk::SamplerCreateInfo samplerInfo = {
		{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = vk::CompareOp::eNever;

	samplerInfo.setMaxLod(FLT_MAX);
	vk::Sampler sampler = VulkanRenderer::CreateSampler(samplerInfo);
	vk::DescriptorImageInfo desc{sampler, textureImageView,
								 vk::ImageLayout::eShaderReadOnlyOptimal};
	s_HdrSkysphere = {desc, imgAllocation};

	Allocator::FlushStaging();
}
