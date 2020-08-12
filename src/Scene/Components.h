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
	glm::mat4 Transform{1.0f};

	TransformComponent() = default;
	TransformComponent(const TransformComponent&) = default;
	TransformComponent& operator=(const TransformComponent&) = default;
	explicit TransformComponent(const glm::mat4& transform)
		: Transform(transform)
	{
	}

	operator glm::mat4&()
	{
		return Transform;
	}
	operator const glm::mat4&() const
	{
		return Transform;
	}
};
struct MeshComponent
{
	uint32_t m_VerticesCount{0};
	uint32_t m_IndicesCount{0};
	std::shared_ptr<BufferAllocation> m_VertexBuffer{};
	std::shared_ptr<BufferAllocation> m_IndexBuffer{};

	MeshComponent() = default;
	MeshComponent(const MeshComponent&) = default;
	MeshComponent& operator=(const MeshComponent&) = default;
	MeshComponent(const uint32_t indicesCount, const uint32_t verticesCount,
				  BufferAllocation* vertexBuffer, BufferAllocation* indexBuffer)
		: m_VerticesCount(verticesCount)
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
	MaterialComponent(BufferAllocation* materialBuffer, std::vector<std::shared_ptr<TextureImage>> textureImages)
		: m_MaterialBuffer(materialBuffer)
		, m_TextureImages(std::move(textureImages))
	{
	}
};
} // namespace Neon

#endif //NEON_COMPONENTS_H
