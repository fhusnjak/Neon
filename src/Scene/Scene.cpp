//
// Created by Filip on 3.8.2020..
//

#include "Scene.h"
#include "VulkanRenderer.h"

#include "Allocator.h"
#include "PerspectiveCameraController.h"

static inline std::string GetFileName(const std::string& path)
{
	int lastDelimiter = -1;
	int i = 0;
	for (char c : path)
	{
		if (c == '\\' || c == '/') { lastDelimiter = i; }
		i++;
	}
	return path.substr(lastDelimiter + 1);
}

Neon::Entity Neon::Scene::CreateEntity(const std::string& name)
{
	Entity entity = {m_Registry.create(), this};
	auto& tag = entity.AddComponent<TagComponent>();
	tag.Tag = name.empty() ? "Entity" : name;
	return entity;
}

Neon::Entity Neon::Scene::LoadAnimatedModel(const std::string& filename)
{
	Assimp::Importer importer;
	const aiScene* scene =
		importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
										aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	assert(scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode);
	assert(scene->HasAnimations());
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Material> materials;
	std::vector<std::shared_ptr<TextureImage>> textureImages;
	std::unordered_map<std::string, uint32_t> boneMap;
	std::vector<glm::mat4> boneOffsets;

	ProcessNode(scene, scene->mRootNode, glm::mat4(1.0f), vertices, indices, materials,
				textureImages, boneMap, boneOffsets);

	Entity entity = CreateEntity(scene->mRootNode->mName.C_Str());
	auto& skinnedMeshRenderer =
		entity.AddComponent<SkinnedMeshRenderer>(scene, 0, boneMap, boneOffsets);
	skinnedMeshRenderer.m_TextureImages = std::move(textureImages);
	auto& transformComponent = entity.AddComponent<Transform>(glm::mat4(1.0));

	auto cmdBuff = VulkanRenderer::BeginSingleTimeCommands();

	skinnedMeshRenderer.m_BoneBuffer = Neon::Allocator::CreateBuffer(
		sizeof(boneOffsets[0]) * skinnedMeshRenderer.m_BoneSize, vk::BufferUsageFlagBits::eStorageBuffer,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	skinnedMeshRenderer.m_Mesh.m_VerticesCount = (uint32_t)vertices.size();
	skinnedMeshRenderer.m_Mesh.m_IndicesCount = (uint32_t)indices.size();
	skinnedMeshRenderer.m_Mesh.m_VertexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, vertices,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	skinnedMeshRenderer.m_Mesh.m_IndexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, indices,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	skinnedMeshRenderer.m_MaterialBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, materials, vk::BufferUsageFlagBits::eStorageBuffer);

	Neon::VulkanRenderer::EndSingleTimeCommands(cmdBuff);

	Neon::VulkanRenderer::LoadAnimatedModel(skinnedMeshRenderer);

	Neon::Allocator::FlushStaging();

	return entity;
}

void Neon::Scene::ProcessNode(const aiScene* scene, aiNode* node, glm::mat4 parentTransform,
							  std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
							  std::vector<Material>& materials,
							  std::vector<std::shared_ptr<TextureImage>>& textureImages,
							  std::unordered_map<std::string, uint32_t>& boneMap,
							  std::vector<glm::mat4>& boneOffsets)
{
	glm::mat4 nodeTransform = parentTransform * glm::transpose(*(glm::mat4*)&node->mTransformation);
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(scene, mesh, nodeTransform, vertices, indices, materials, textureImages,
					boneMap, boneOffsets);
	}
	for (int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(scene, node->mChildren[i], nodeTransform, vertices, indices, materials,
					textureImages, boneMap, boneOffsets);
	}
}

void Neon::Scene::ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 transform,
							  std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
							  std::vector<Material>& materials,
							  std::vector<std::shared_ptr<TextureImage>>& textureImages,
							  std::unordered_map<std::string, uint32_t>& boneMap,
							  std::vector<glm::mat4>& boneOffsets)
{
	int meshSizeBefore = vertices.size();
	int newIndicesCount = 0;
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		// FIXME: for now just ignore faces with number of indices not equal to 3
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3) { continue; }
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j] + meshSizeBefore);
			newIndicesCount++;
		}
	}
	if (newIndicesCount == 0) { return; }

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
		for (int j = 0; j < MAX_BONES_PER_VERTEX; j++)
		{
			vertex.boneIDs[j] = 0;
			vertex.boneWeights[j] = 0;
		}
		vertices.push_back(vertex);
	}

	for (int i = 0; i < mesh->mNumBones; i++)
	{
		aiBone* bone = mesh->mBones[i];
		uint32_t boneIndex = boneOffsets.size();
		if (boneMap.find(bone->mName.C_Str()) != boneMap.end())
		{ boneIndex = boneMap[bone->mName.C_Str()]; }
		else
		{
			boneOffsets.push_back(glm::transpose(*(glm::mat4*)&(bone->mOffsetMatrix)));
			boneMap[bone->mName.C_Str()] = boneIndex;
		}
		for (int j = 0; j < bone->mNumWeights; j++)
		{
			aiVertexWeight weight = bone->mWeights[j];
			assert(weight.mVertexId + meshSizeBefore < vertices.size());
			int k = 0;
			while (vertices[weight.mVertexId + meshSizeBefore].boneWeights[k] > 0)
			{
				k++;
				assert(k < MAX_BONES_PER_VERTEX);
			}
			vertices[weight.mVertexId + meshSizeBefore].boneIDs[k] = boneIndex;
			vertices[weight.mVertexId + meshSizeBefore].boneWeights[k] = weight.mWeight;
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

	std::unique_ptr<ImageAllocation> imageAllocation;
	if (aiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		// TODO: for now just load first diffuse texture
		aiString txt;
		aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &txt);
		std::string texturePath = "textures/" + GetFileName(txt.C_Str());
		imageAllocation = Neon::Allocator::CreateTextureImage(texturePath);
	}
	else
	{
		int texWidth = 1, texHeight = 1;
		auto* color = new glm::u8vec4(255, 255, 255, 255);
		auto* pixels = reinterpret_cast<stbi_uc*>(color);
		imageAllocation = Neon::Allocator::CreateTextureImage(pixels, texWidth, texHeight);
	}
	assert(imageAllocation);
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

void Neon::Scene::OnUpdate(float ts, Neon::PerspectiveCameraController controller,
						   glm::vec4 clearColor, glm::vec3 lightPosition)
{
	VulkanRenderer::BeginScene(controller.GetCamera(), clearColor);R
	auto group = m_Registry.group<Transform>(entt::get<SkinnedMeshRenderer>);
	for (auto entity : group)
	{
		const auto& [transform, skinnedMeshRenderer] =
			group.get<Transform, SkinnedMeshRenderer>(entity);
		skinnedMeshRenderer.Update(ts / 1000.0f);
		VulkanRenderer::Render(transform, skinnedMeshRenderer, lightPosition);
	}
	VulkanRenderer::EndScene();
}
