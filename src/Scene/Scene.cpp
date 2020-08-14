//
// Created by Filip on 3.8.2020..
//

#include "Scene.h"
#include "VulkanRenderer.h"

#include "Allocator.h"
#include "PerspectiveCameraController.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

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
	Assimp::Importer importer;
	const aiScene* scene =
		importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
										aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	assert(scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode);

	Entity entity = CreateEntity(name);
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Material> materials;

	auto& materialComponent = entity.AddComponent<MaterialComponent>();
	ProcessNode(scene->mRootNode, scene, vertices, indices, materials,
				materialComponent.m_TextureImages);

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

	materialComponent.m_MaterialBuffer = Neon::Allocator::CreateDeviceLocalBuffer(
		cmdBuff, materials, vk::BufferUsageFlagBits::eStorageBuffer);

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

void Neon::Scene::ProcessNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices,
							  std::vector<uint32_t>& indices, std::vector<Material>& materials,
							  std::vector<std::shared_ptr<TextureImage>>& textureImages)
{
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, vertices, indices, materials, textureImages);
	}
	for (int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, vertices, indices, materials, textureImages);
	}
}

void Neon::Scene::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices,
							  std::vector<uint32_t>& indices, std::vector<Material>& materials,
							  std::vector<std::shared_ptr<TextureImage>>& textureImages)
{
	size_t meshSizeBefore = vertices.size();
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex{};
		vertex.pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
		vertex.norm = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
		vertex.matID = materials.size();
		// TODO: for now just use first texture coordinate
		if (mesh->mTextureCoords[0])
		{ vertex.texCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}; }
		else
		{
			vertex.texCoord = {0.0f, 0.0f};
		}
		vertices.push_back(vertex);
	}
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		// FIXME: for now just ignore faces with number of indices not equal to 3
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3) continue;
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j] + meshSizeBefore);
		}
	}
	aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
	Material material{};
	aiColor3D ambientColor(0.f, 0.f, 0.f);
	aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
	material.ambient = {ambientColor.r, ambientColor.g, ambientColor.b};
	aiColor3D diffuseColor(0.f, 0.f, 0.f);
	aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
	material.diffuse = {diffuseColor.r, diffuseColor.g, diffuseColor.b};
	aiColor3D specularColor(0.f, 0.f, 0.f);
	aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
	material.specular = {specularColor.r, specularColor.g, specularColor.b};
	float shininess;
	aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
	material.shininess = shininess;
	material.textureID = textureImages.size();
	materials.push_back(material);
	if (aiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		// TODO: for now just load first diffuse texture
		aiString txt;
		aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &txt);
		std::string texturePath = txt.C_Str();
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
		textureImages.push_back(std::shared_ptr<TextureImage>(textureImage));
	}
	else
	{
		int texWidth, texHeight;
		texWidth = texHeight = 1;
		auto* color = new glm::u8vec4(255, 255, 255, 128);
		auto* pixels = reinterpret_cast<stbi_uc*>(color);
		std::unique_ptr<ImageAllocation> imageAllocation =
			Neon::Allocator::CreateTextureImage(pixels, texWidth, texHeight);
		vk::ImageView textureImageView = Neon::VulkanRenderer::CreateImageView(
			imageAllocation->image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
		vk::SamplerCreateInfo samplerInfo = {
			{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
		samplerInfo.setMaxLod(FLT_MAX);
		vk::Sampler sampler = Neon::VulkanRenderer::CreateSampler(samplerInfo);
		vk::DescriptorImageInfo desc{sampler, textureImageView,
									 vk::ImageLayout::eShaderReadOnlyOptimal};
		auto* textureImage = new TextureImage{desc, std::move(imageAllocation)};
		textureImages.push_back(std::shared_ptr<TextureImage>(textureImage));
	}
}
