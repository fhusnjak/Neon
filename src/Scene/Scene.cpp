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

glm::mat4 globalInverseTransform;

Neon::Entity Neon::Scene::CreateEntity(const std::string& name)
{
	Entity entity = {m_Registry.create(), this};
	auto& tag = entity.AddComponent<TagComponent>();
	tag.Tag = name.empty() ? "Entity" : name;
	return entity;
}


float currentAnimationTime = 0;
float ticksPerSecond;
float duration;

void Neon::Scene::LoadAnimatedModel(const std::string& filename, const std::string& name)
{
	const aiScene* scene =
		m_Importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
										  aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	assert(scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode);
	if (scene->HasAnimations())
	{
		ticksPerSecond = scene->mAnimations[0]->mTicksPerSecond != 0 ?
						 scene->mAnimations[0]->mTicksPerSecond : 25.0f;
		duration = scene->mAnimations[0]->mDuration;
	}
	globalInverseTransform = glm::inverse(*(glm::mat4*)&scene->mRootNode->mTransformation);
	ProcessNode(scene, scene->mRootNode, glm::mat4(1.0f));
}

void Neon::Scene::ProcessNode(const aiScene* scene, aiNode* node, glm::mat4 parentTransform)
{
	glm::mat4 nodeTransform = parentTransform * glm::transpose(*(glm::mat4*)&node->mTransformation);
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(scene, mesh, nodeTransform);
	}
	for (int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(scene, node->mChildren[i], nodeTransform);
	}
}

void Neon::Scene::ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 transform)
{
	std::vector<Vertex> vertices;
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex{};
		vertex.pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
		//vertex.norm = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
		vertex.matID = 0;
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

	std::vector<uint32_t> indices;
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		// FIXME: for now just ignore faces with number of indices not equal to 3
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3)
		{
			continue;
		}
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	if (indices.empty())
	{
		return;
	}

	Entity entity = CreateEntity(mesh->mName.C_Str());
	auto& meshComponent = entity.AddComponent<MeshComponent>(scene);
	auto& materialComponent = entity.AddComponent<MaterialComponent>();
	auto& transformComponent = entity.AddComponent<TransformComponent>();

	for (int i = 0; i < mesh->mNumBones; i++)
	{
		aiBone* bone = mesh->mBones[i];
		meshComponent.m_BoneMapping[bone->mName.C_Str()] = meshComponent.m_BoneOffsets.size();
		for (int j = 0; j < bone->mNumWeights; j++)
		{
			aiVertexWeight weight = bone->mWeights[j];
			assert(weight.mVertexId < vertices.size());
			int k = 0;
			while (vertices[weight.mVertexId].boneWeights[k] > 0)
			{
				k++;
				assert(k < MAX_BONES_PER_VERTEX);
			}
			vertices[weight.mVertexId].boneIDs[k] = meshComponent.m_BoneOffsets.size();
			vertices[weight.mVertexId].boneWeights[k] = weight.mWeight;
		}
		meshComponent.m_BoneOffsets.push_back(glm::transpose(*(glm::mat4*)&(bone->mOffsetMatrix)));
	}

	std::vector<Material> materials;
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
	material.textureID = 0;
	materials.push_back(material);

	transformComponent.m_Transform = glm::scale(glm::mat4(1.0f), {0.01, 0.01, 0.01});
	if (!mesh->HasBones())
	{
		transformComponent.m_Transform = transformComponent.m_Transform * transform;
	}
	transformComponent.m_Transform =
		glm::rotate(glm::mat4(1.0), 3.14f, {0, 1, 0}) * transformComponent.m_Transform;
	transformComponent.m_Transform =
		glm::translate(glm::mat4(1.0), {-5, -1.0, 0}) * transformComponent.m_Transform;

	std::unique_ptr<ImageAllocation> imageAllocation;
	if (aiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		// TODO: for now just load first diffuse texture
		aiString txt;
		aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &txt);
		std::string texturePath = "textures/" + GetFileName(txt.C_Str());
		imageAllocation =
			Neon::Allocator::CreateTextureImage(texturePath);
	}
	else
	{
		int texWidth = 1, texHeight = 1;
		auto* color = new glm::u8vec4(255, 255, 255, 255);
		auto* pixels = reinterpret_cast<stbi_uc*>(color);
		imageAllocation =
			Neon::Allocator::CreateTextureImage(pixels, texWidth, texHeight);
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
	materialComponent.m_TextureImages.push_back(std::shared_ptr<TextureImage>(textureImage));

	auto cmdBuff = VulkanRenderer::BeginSingleTimeCommands();

	if (!meshComponent.m_BoneOffsets.empty())
	{
		meshComponent.m_BoneBuffer = Neon::Allocator::CreateBuffer(
			sizeof(meshComponent.m_BoneOffsets[0]) * meshComponent.m_BoneOffsets.size(), vk::BufferUsageFlagBits::eStorageBuffer,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	meshComponent.m_VerticesCount = (uint32_t)vertices.size();
	meshComponent.m_IndicesCount = (uint32_t)indices.size();
	meshComponent.m_VertexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, vertices,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	meshComponent.m_IndexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, indices,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	materialComponent.m_MaterialBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, materials, vk::BufferUsageFlagBits::eStorageBuffer);

	Neon::VulkanRenderer::EndSingleTimeCommands(cmdBuff);

	if (mesh->HasBones())
	{
		Neon::VulkanRenderer::CreateWavefrontEntity(meshComponent, materialComponent);
	}
	else
	{
		Neon::VulkanRenderer::CreateWavefrontEntity(meshComponent, materialComponent);
	}

	Neon::Allocator::FlushStaging();

	m_AnimatedModel.parts.push_back(entity);
}


const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
{
	for (int i = 0; i < animation->mNumChannels; i++)
	{
		const aiNodeAnim* pNodeAnim = animation->mChannels[i];
		if (std::string(pNodeAnim->mNodeName.data) == nodeName) { return pNodeAnim; }
	}
	return nullptr;
}

int FindScaling(float animationTime, const aiNodeAnim* nodeAnim)
{
	assert(nodeAnim->mNumScalingKeys > 0);
	for (int i = 0; i < nodeAnim->mNumScalingKeys - 1; i++)
	{
		if (animationTime < (float)nodeAnim->mScalingKeys[i + 1].mTime) { return i; }
	}
	assert(0);
}

int FindPosition(float animationTime, const aiNodeAnim* nodeAnim)
{
	assert(nodeAnim->mNumPositionKeys > 0);
	for (int i = 0; i < nodeAnim->mNumPositionKeys - 1; i++)
	{
		if (animationTime < (float)nodeAnim->mPositionKeys[i + 1].mTime) { return i; }
	}
	assert(0);
}

int FindRotation(float animationTime, const aiNodeAnim* nodeAnim)
{
	assert(nodeAnim->mNumRotationKeys > 0);
	for (int i = 0; i < nodeAnim->mNumRotationKeys - 1; i++)
	{
		if (animationTime < (float)nodeAnim->mRotationKeys[i + 1].mTime) { return i; }
	}
	assert(0);
}

void CalcInterpolatedScaling(glm::vec3& out, float animationTime, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumScalingKeys == 1)
	{
		out = {nodeAnim->mScalingKeys[0].mValue.x, nodeAnim->mScalingKeys[0].mValue.y,
			   nodeAnim->mScalingKeys[0].mValue.z};
		return;
	}
	int scalingIndex = FindScaling(animationTime, nodeAnim);
	int nextScalingIndex = (scalingIndex + 1);
	assert(nextScalingIndex < nodeAnim->mNumScalingKeys);
	auto deltaTime = static_cast<float>(nodeAnim->mScalingKeys[nextScalingIndex].mTime -
										nodeAnim->mScalingKeys[scalingIndex].mTime);
	float factor = (animationTime - (float)nodeAnim->mScalingKeys[scalingIndex].mTime) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	const glm::vec3& startScaling = {nodeAnim->mScalingKeys[scalingIndex].mValue.x,
									 nodeAnim->mScalingKeys[scalingIndex].mValue.y,
									 nodeAnim->mScalingKeys[scalingIndex].mValue.z};
	const glm::vec3& endScaling = {nodeAnim->mScalingKeys[nextScalingIndex].mValue.x,
								   nodeAnim->mScalingKeys[nextScalingIndex].mValue.y,
								   nodeAnim->mScalingKeys[nextScalingIndex].mValue.z};
	out = factor * endScaling + (1.0f - factor) * startScaling;
}

void CalcInterpolatedPosition(glm::vec3& out, float animationTime, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumPositionKeys == 1)
	{
		out = {nodeAnim->mPositionKeys[0].mValue.x, nodeAnim->mPositionKeys[0].mValue.y,
			   nodeAnim->mPositionKeys[0].mValue.z};
		return;
	}
	int positionIndex = FindPosition(animationTime, nodeAnim);
	int nextPositionIndex = (positionIndex + 1);
	assert(nextPositionIndex < nodeAnim->mNumPositionKeys);
	auto deltaTime = static_cast<float>(nodeAnim->mPositionKeys[nextPositionIndex].mTime -
										nodeAnim->mPositionKeys[positionIndex].mTime);
	float factor =
		(animationTime - (float)nodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	const glm::vec3& startPosition = {nodeAnim->mPositionKeys[positionIndex].mValue.x,
									  nodeAnim->mPositionKeys[positionIndex].mValue.y,
									  nodeAnim->mPositionKeys[positionIndex].mValue.z};
	const glm::vec3& endPosition = {nodeAnim->mPositionKeys[nextPositionIndex].mValue.x,
									nodeAnim->mPositionKeys[nextPositionIndex].mValue.y,
									nodeAnim->mPositionKeys[nextPositionIndex].mValue.z};
	out = factor * endPosition + (1.0f - factor) * startPosition;
}

void CalcInterpolatedRotation(aiQuaternion& out, float animationTime, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumRotationKeys == 1)
	{
		out = nodeAnim->mRotationKeys[0].mValue;
		return;
	}
	int rotationIndex = FindRotation(animationTime, nodeAnim);
	int nextRotationIndex = (rotationIndex + 1);
	assert(nextRotationIndex < nodeAnim->mNumRotationKeys);
	auto deltaTime = static_cast<float>(nodeAnim->mRotationKeys[nextRotationIndex].mTime -
										nodeAnim->mRotationKeys[rotationIndex].mTime);
	float factor =
		(animationTime - (float)nodeAnim->mRotationKeys[rotationIndex].mTime) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[rotationIndex].mValue;
	const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[nextRotationIndex].mValue;
	aiQuaternion::Interpolate(out, startRotationQ, endRotationQ, factor);
	out = out.Normalize();
}

void ReadNodeHierarchy(const aiScene* scene, float animationTime, const aiNode* pNode,
					   const glm::mat4& parentTransform, std::vector<glm::mat4>& transformations,
					   std::unordered_map<std::string, uint32_t>& boneMapping,
					   const std::vector<glm::mat4>& boneOffsets)
{
	std::string nodeName = pNode->mName.data;

	const aiAnimation* animation = scene->mAnimations[0];

	glm::mat4 nodeTransformation = glm::transpose(*(glm::mat4*)&pNode->mTransformation);

	const aiNodeAnim* nodeAnim = FindNodeAnim(animation, nodeName);

	if (nodeAnim)
	{
		glm::vec3 scaling;
		CalcInterpolatedScaling(scaling, animationTime, nodeAnim);
		glm::mat4 scalingMat = glm::scale(glm::mat4(1.0), scaling);

		glm::vec3 translation;
		CalcInterpolatedPosition(translation, animationTime, nodeAnim);
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0), translation);

		aiQuaternion rotationQ;
		CalcInterpolatedRotation(rotationQ, animationTime, nodeAnim);
		auto mat = rotationQ.GetMatrix();
		auto rotationMat = glm::mat4(glm::transpose(*(glm::mat3*)&mat));

		nodeTransformation = translationMat * rotationMat * scalingMat;
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransformation;

	if (boneMapping.find(nodeName) != boneMapping.end())
	{
		transformations[boneMapping[nodeName]] = globalTransformation * boneOffsets[boneMapping[nodeName]];
	}

	for (int i = 0; i < pNode->mNumChildren; i++)
	{
		ReadNodeHierarchy(scene, animationTime, pNode->mChildren[i], globalTransformation,
						  transformations, boneMapping, boneOffsets);
	}
}

void Neon::Scene::OnUpdate(float ts, Neon::PerspectiveCameraController controller,
						   glm::vec4 clearColor, glm::vec3 lightPosition)
{
	VulkanRenderer::BeginScene(controller.GetCamera(), clearColor);
	auto group = m_Registry.group<TransformComponent>(entt::get<MeshComponent, MaterialComponent>);
	for (auto entity : group)
	{
		const auto& [transform, mesh, material] =
		group.get<TransformComponent, MeshComponent, MaterialComponent>(entity);
		if (mesh.m_BoneBuffer)
		{
			std::vector<glm::mat4> transformations(mesh.m_BoneOffsets.size());
			ReadNodeHierarchy(mesh.m_Scene, currentAnimationTime, mesh.m_Scene->mRootNode, glm::mat4(1.0f), transformations, mesh.m_BoneMapping, mesh.m_BoneOffsets);
			Allocator::UpdateAllocation(mesh.m_BoneBuffer->allocation, transformations);
		}
		VulkanRenderer::Render(transform, mesh, material, lightPosition);
	}
	VulkanRenderer::EndScene();
	currentAnimationTime += ts / 1000.0f * ticksPerSecond;
	currentAnimationTime = fmod(currentAnimationTime, duration);
}
