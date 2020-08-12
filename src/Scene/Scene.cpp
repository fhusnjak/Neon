//
// Created by Filip on 3.8.2020..
//

#include "Scene.h"
#include "VulkanRenderer.h"

#include "Allocator.h"
#include "Core/Core.h"
#include "PerspectiveCameraController.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

static inline std::string GetPath(const std::string& file)
{
	std::string dir;
	size_t idx = file.find_last_of("\\/");
	if (idx != std::string::npos) dir = file.substr(0, idx);
	if (!dir.empty()) { dir += "\\"; }
	return dir;
}

Neon::Entity Neon::Scene::CreateEntity(const std::string& name)
{
	Entity entity = {m_Registry.create(), this};
	auto& tag = entity.AddComponent<TagComponent>();
	tag.Tag = name.empty() ? "Entity" : name;
	return entity;
}

Neon::Entity Neon::Scene::CreateWavefrontEntity(const std::string& filename,
												const std::string& name)
{
	Entity entity = CreateEntity(name);
	std::vector<Material> materials;
	std::vector<std::string> textures;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> tinyMaterials;
	std::string warn, err;

	result_assert(tinyobj::LoadObj(&attrib, &shapes, &tinyMaterials, &warn, &err, filename.c_str(),
								   GetPath(filename).c_str()))

		for (const auto& material : tinyMaterials)
	{
		Material m = {};
		m.ambient = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]);
		m.diffuse = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
		m.specular = glm::vec3(material.specular[0], material.specular[1], material.specular[2]);
		m.emission = glm::vec3(material.emission[0], material.emission[1], material.emission[2]);
		m.transmittance = glm::vec3(material.transmittance[0], material.transmittance[1],
									material.transmittance[2]);
		m.dissolve = material.dissolve;
		m.ior = material.ior;
		m.shininess = material.shininess;
		m.illum = material.illum;
		if (!material.diffuse_texname.empty())
		{
			textures.push_back(material.diffuse_texname);
			m.textureID = static_cast<int>(textures.size()) - 1;
		}

		materials.emplace_back(m);
	}

	if (materials.empty()) { materials.emplace_back(Material()); }

	// Converting from Srgb to linear
	for (auto& m : materials)
	{
		m.ambient = glm::pow(m.ambient, glm::vec3(2.2f));
		m.diffuse = glm::pow(m.diffuse, glm::vec3(2.2f));
		m.specular = glm::pow(m.specular, glm::vec3(2.2f));
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (auto& shape : shapes)
	{
		uint32_t faceID = 0;
		int indexCount = 0;
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};
			vertex.pos = {attrib.vertices[3L * static_cast<uint64_t>(index.vertex_index) + 0L],
						  attrib.vertices[3L * static_cast<uint64_t>(index.vertex_index) + 1L],
						  attrib.vertices[3L * static_cast<uint64_t>(index.vertex_index) + 2L]};

			if (!attrib.normals.empty() && index.normal_index >= 0)
			{
				vertex.norm = {attrib.normals[3L * static_cast<uint64_t>(index.normal_index) + 0L],
							   attrib.normals[3L * static_cast<uint64_t>(index.normal_index) + 1L],
							   attrib.normals[3L * static_cast<uint64_t>(index.normal_index) + 2L]};
			}

			if (!attrib.colors.empty())
			{
				vertex.color = {attrib.colors[3L * static_cast<uint64_t>(index.vertex_index) + 0L],
								attrib.colors[3L * static_cast<uint64_t>(index.vertex_index) + 1L],
								attrib.colors[3L * static_cast<uint64_t>(index.vertex_index) + 2L]};
			}

			if (!attrib.texcoords.empty() && index.texcoord_index >= 0)
			{
				vertex.texCoord = {
					attrib.texcoords[2L * static_cast<uint64_t>(index.texcoord_index) + 0L],
					1.0f - attrib.texcoords[2L * static_cast<uint64_t>(index.texcoord_index) + 1L]};
			}

			vertex.matID = shape.mesh.material_ids[faceID];
			if (vertex.matID < 0 || vertex.matID >= materials.size()) { vertex.matID = 0; }
			indexCount++;
			if (indexCount >= 3)
			{
				++faceID;
				indexCount = 0;
			}

			vertices.push_back(vertex);
			indices.push_back(static_cast<int>(indices.size()));
		}
	}

	std::vector<std::vector<glm::vec3>> normals(vertices.size());
	if (attrib.normals.empty())
	{
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			Vertex& v0 = vertices[indices[i + 0]];
			Vertex& v1 = vertices[indices[i + 1]];
			Vertex& v2 = vertices[indices[i + 2]];

			glm::vec3 n = glm::normalize(glm::cross((v1.pos - v0.pos), (v2.pos - v0.pos)));
			v0.norm = n;
			v1.norm = n;
			v2.norm = n;
		}
	}

	auto cmdBuff = Neon::VulkanRenderer::BeginSingleTimeCommands();
	auto& meshComponent = entity.AddComponent<MeshComponent>();
	meshComponent.m_VerticesCount = (uint32_t)vertices.size();
	meshComponent.m_IndicesCount = (uint32_t)indices.size();
	meshComponent.m_VertexBuffer = Neon::Allocator::CreateDeviceLocalBuffer(
		cmdBuff, vertices,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	meshComponent.m_IndexBuffer = Neon::Allocator::CreateDeviceLocalBuffer(
		cmdBuff, indices,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	auto& materialComponent = entity.AddComponent<MaterialComponent>();
	materialComponent.m_MaterialBuffer = Neon::Allocator::CreateDeviceLocalBuffer(
		cmdBuff, materials, vk::BufferUsageFlagBits::eStorageBuffer);
	for (auto& texturePath : textures)
	{
		std::unique_ptr<ImageAllocation> imageAllocation =
			Neon::Allocator::CreateTextureImage(texturePath);
		vk::ImageView textureImageView = Neon::VulkanRenderer::CreateImageView(
			imageAllocation->image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
		vk::SamplerCreateInfo samplerInfo = {
			{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
		samplerInfo.setMaxLod(FLT_MAX);
		vk::Sampler sampler = Neon::VulkanRenderer::CreateSampler(samplerInfo);
		vk::DescriptorImageInfo desc{sampler, textureImageView,
									 vk::ImageLayout::eShaderReadOnlyOptimal};
		auto* textureImage = new TextureImage{desc, std::move(imageAllocation)};
		materialComponent.m_TextureImages.push_back(std::shared_ptr<TextureImage>(textureImage));
	}
	if (textures.empty())
	{
		int texWidth, texHeight;
		texWidth = texHeight = 1;
		auto* color = new glm::u8vec4(255, 255, 255, 128);
		auto* pixels = reinterpret_cast<stbi_uc*>(color);
		std::unique_ptr<ImageAllocation> imageAllocation = Neon::Allocator::CreateTextureImage(pixels, texWidth, texHeight, 4);
		vk::ImageView textureImageView = Neon::VulkanRenderer::CreateImageView(
			imageAllocation->image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
		vk::SamplerCreateInfo samplerInfo = {
			{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
		samplerInfo.setMaxLod(FLT_MAX);
		vk::Sampler sampler = Neon::VulkanRenderer::CreateSampler(samplerInfo);
		vk::DescriptorImageInfo desc{sampler, textureImageView,
									 vk::ImageLayout::eShaderReadOnlyOptimal};
		auto* textureImage = new TextureImage{desc, std::move(imageAllocation)};
		materialComponent.m_TextureImages.push_back(std::shared_ptr<TextureImage>(textureImage));
	}

	Neon::VulkanRenderer::EndSingleTimeCommands(cmdBuff);

	Neon::VulkanRenderer::CreateWavefrontEntity(materialComponent);

	Neon::Allocator::FlushStaging();

	return entity;
}

void Neon::Scene::OnUpdate(float ts, Neon::PerspectiveCameraController controller,
						   glm::vec4 clearColor, glm::vec3 lightPosition)
{
	Neon::VulkanRenderer::BeginScene(controller.GetCamera(), clearColor);
	auto group = m_Registry.group<TransformComponent>(entt::get<MeshComponent, MaterialComponent>);
	for (auto entity : group)
	{
		const auto& [transform, mesh, material] =
			group.get<TransformComponent, MeshComponent, MaterialComponent>(entity);
		Neon::VulkanRenderer::Render(transform, mesh, material, lightPosition);
	}
	Neon::VulkanRenderer::EndScene();
}
