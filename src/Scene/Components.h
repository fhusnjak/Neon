//
// Created by Filip on 3.8.2020..
//

#ifndef NEON_COMPONENTS_H
#define NEON_COMPONENTS_H

#include "Allocator.h"
#include "Animation.h"
#include "DescriptorSet.h"
#include "GraphicsPipeline.h"
#include <Core/Allocator.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/GraphicsPipeline.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/postprocess.h>

namespace Neon
{
struct TagComponent
{
	std::string Tag;

	TagComponent() = default;
	explicit TagComponent(std::string tag)
		: Tag(std::move(tag))
	{
	}
};

struct Transform
{
	glm::mat4 m_Transform{1.0f};

	Transform() = default;
	explicit Transform(const glm::mat4& transform)
		: m_Transform(transform)
	{
	}
};

struct Mesh
{
	uint32_t m_VerticesCount{0};
	uint32_t m_IndicesCount{0};
	std::unique_ptr<BufferAllocation> m_VertexBuffer{};
	std::unique_ptr<BufferAllocation> m_IndexBuffer{};
};

struct SkyDomeRenderer
{
	Mesh m_Mesh;

	GraphicsPipeline m_GraphicsPipeline;
	std::vector<DescriptorSet> m_DescriptorSets;

	SkyDomeRenderer() = default;
};

struct SkinnedMeshRenderer
{
	Mesh m_Mesh;

	Bone m_RootBone;
	uint32_t m_BoneSize;
	std::unique_ptr<BufferAllocation> m_BoneBuffer{};

	std::unique_ptr<Animation> m_Animation = nullptr;

	GraphicsPipeline m_GraphicsPipeline;
	std::vector<DescriptorSet> m_DescriptorSets;

	std::shared_ptr<BufferAllocation> m_MaterialBuffer{};
	std::vector<TextureImage> m_TextureImages;

	SkinnedMeshRenderer(const aiScene* scene, int index,
						std::unordered_map<std::string, uint32_t>& boneMap,
						const std::vector<glm::mat4>& offsetMatrices)
		: m_BoneSize(offsetMatrices.size())
	{
		CreateBoneTree(scene, scene->mRootNode, m_RootBone, glm::mat4(1.0), index, boneMap,
					   offsetMatrices, m_BoneSize);
		assert(m_RootBone.GetID() >= 0);
		m_Animation = std::make_unique<Animation>(scene, index, boneMap, m_BoneSize);
	}

	void Update(float seconds)
	{
		std::vector<glm::mat4> transforms(m_BoneSize);
		m_Animation->Update(seconds, transforms, m_RootBone);
		Allocator::UpdateAllocation(m_BoneBuffer->m_Allocation, transforms);
	}

	void CreateBoneTree(const aiScene* scene, const aiNode* node, Bone& parentBone,
						glm::mat4 parentTransform, int animationIndex,
						std::unordered_map<std::string, uint32_t>& boneMap,
						const std::vector<glm::mat4>& offsetMatrices, uint32_t& bonesCount)
	{
		std::string nodeName = node->mName.data;

		const aiAnimation* animation = scene->mAnimations[animationIndex];

		const aiNodeAnim* nodeAnim = nullptr;
		for (int i = 0; i < animation->mNumChannels; i++)
		{
			const aiNodeAnim* nodeAnimTemp = animation->mChannels[i];
			if (std::string(nodeAnimTemp->mNodeName.data) == nodeName)
			{
				nodeAnim = nodeAnimTemp;
				break;
			}
		}

		glm::mat4 localTransform = glm::transpose(*(glm::mat4*)&node->mTransformation);
		glm::mat4 newParentTransform = parentTransform * localTransform;

		Bone* newParentBone = &parentBone;
		if (nodeAnim || boneMap.find(nodeName) != boneMap.end())
		{
			newParentTransform = glm::mat4(1.0);
			Bone newBone;
			if (boneMap.find(nodeName) == boneMap.end())
			{
				newBone = Bone(bonesCount, glm::mat4(1.0), parentTransform, localTransform);
				boneMap[nodeName] = bonesCount++;
			}
			else
			{
				newBone = Bone(boneMap[nodeName], offsetMatrices[boneMap[nodeName]],
							   parentTransform, localTransform);
			}
			newBone.m_Animated = nodeAnim;
			if (parentBone.GetID() == -1)
			{
				parentBone = newBone;
				newParentBone = &parentBone;
			}
			else
			{
				parentBone.GetChildren().push_back(newBone);
				newParentBone = &parentBone.GetChildren().back();
			}
		}

		for (int i = 0; i < node->mNumChildren; i++)
		{
			CreateBoneTree(scene, node->mChildren[i], *newParentBone, newParentTransform,
						   animationIndex, boneMap, offsetMatrices, bonesCount);
		}
	}
};

struct MeshRenderer
{
	Mesh m_Mesh;

	GraphicsPipeline m_GraphicsPipeline;
	std::vector<DescriptorSet> m_DescriptorSets;

	std::shared_ptr<BufferAllocation> m_MaterialBuffer{};
	std::vector<TextureImage> m_TextureImages;

	MeshRenderer() = default;
};

struct TerrainRenderer
{
	Mesh m_Mesh;

	GraphicsPipeline m_GraphicsPipeline;
	std::vector<DescriptorSet> m_DescriptorSets;

	std::unique_ptr<BufferAllocation> m_MaterialBuffer{};
	TextureImage m_BlendMap;
	TextureImage m_BackgroundTexture;
	TextureImage m_RTexture;
	TextureImage m_GTexture;
	TextureImage m_BTexture;

	TerrainRenderer() = default;
};

struct Water
{
	Mesh m_Mesh;

	GraphicsPipeline m_GraphicsPipeline;
	std::vector<DescriptorSet> m_DescriptorSets;

	std::unique_ptr<BufferAllocation> m_MaterialBuffer{};
	std::vector<TextureImage> m_TextureImages;

	// Used for multisampling
	TextureImage m_SampledImage;
	TextureImage m_DepthReflectionTexture;
	TextureImage m_ReflectionTexture;

	std::vector<vk::UniqueFramebuffer> m_FrameBuffers;

	explicit Water(vk::Extent2D extent);
};
} // namespace Neon

#endif //NEON_COMPONENTS_H
