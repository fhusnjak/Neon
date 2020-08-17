//
// Created by Filip on 3.8.2020..
//

#ifndef NEON_COMPONENTS_H
#define NEON_COMPONENTS_H

#include "Allocator.h"
#include "DescriptorSet.h"
#include "GraphicsPipeline.h"
#include <Core/Allocator.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/GraphicsPipeline.h>
#include <glm/glm.hpp>
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
	TagComponent(const TagComponent&) = default;
	TagComponent& operator=(const TagComponent&) = default;
	explicit TagComponent(std::string tag)
		: Tag(std::move(tag))
	{
	}
};
struct TransformComponent
{
	glm::mat4 m_Transform{1.0f};

	TransformComponent() = default;
	TransformComponent(const TransformComponent&) = default;
	TransformComponent& operator=(const TransformComponent&) = default;
	explicit TransformComponent(const glm::mat4& transform)
		: m_Transform(transform)
	{
	}

	operator glm::mat4&()
	{
		return m_Transform;
	}
	operator const glm::mat4&() const
	{
		return m_Transform;
	}
};

struct MeshComponent
{
	// TODO: Create special component for these
	const aiScene* m_Scene;
	std::unordered_map<std::string, uint32_t> m_BoneMapping;
	std::vector<glm::mat4> m_BoneOffsets;
	std::shared_ptr<BufferAllocation> m_BoneBuffer{};

	uint32_t m_VerticesCount{0};
	uint32_t m_IndicesCount{0};
	std::shared_ptr<BufferAllocation> m_VertexBuffer{};
	std::shared_ptr<BufferAllocation> m_IndexBuffer{};

	MeshComponent(const aiScene* scene)
		: m_Scene(scene)
	{
	}
	MeshComponent(const MeshComponent&) = default;
	MeshComponent& operator=(const MeshComponent&) = default;
	MeshComponent(const aiScene* scene, const uint32_t indicesCount, const uint32_t verticesCount,
				  BufferAllocation* vertexBuffer, BufferAllocation* indexBuffer)
		: m_Scene(scene)
		, m_VerticesCount(verticesCount)
		, m_IndicesCount(indicesCount)
		, m_VertexBuffer(vertexBuffer)
		, m_IndexBuffer(indexBuffer)
	{
	}
};

struct MaterialComponent
{
	std::shared_ptr<BufferAllocation> m_MaterialBuffer{};
	std::vector<std::shared_ptr<TextureImage>> m_TextureImages;
	std::vector<DescriptorSet> m_DescriptorSets;
	GraphicsPipeline graphicsPipeline;

	MaterialComponent() = default;
	MaterialComponent(BufferAllocation* materialBuffer,
					  std::vector<std::shared_ptr<TextureImage>> textureImages)
		: m_MaterialBuffer(materialBuffer)
		, m_TextureImages(std::move(textureImages))
	{
	}
};
} // namespace Neon

#endif //NEON_COMPONENTS_H
