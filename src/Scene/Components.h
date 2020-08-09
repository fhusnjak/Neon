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

struct TagComponent
{
	std::string Tag;

	TagComponent() = default;
	TagComponent(const TagComponent&) = default;
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
	Neon::BufferAllocation m_VertexBuffer{};
	Neon::BufferAllocation m_IndexBuffer{};

	MeshComponent() = default;
	MeshComponent(const MeshComponent&) = default;
	MeshComponent(const uint32_t indicesCount, const uint32_t verticesCount,
				  const Neon::BufferAllocation& vertexBuffer,
				  const Neon::BufferAllocation indexBuffer)
		: m_VerticesCount(verticesCount)
		, m_IndicesCount(indicesCount)
		, m_VertexBuffer(vertexBuffer)
		, m_IndexBuffer(indexBuffer)
	{
	}
};

struct MaterialComponent
{
	Neon::BufferAllocation m_MaterialBuffer{};
	std::vector<Neon::TextureImage> m_TextureImages;
	std::vector<Neon::DescriptorSet> m_DescriptorSets;
	Neon::GraphicsPipeline graphicsPipeline;

	MaterialComponent() = default;
	MaterialComponent(const Neon::BufferAllocation& materialBuffer,
					  std::vector<Neon::TextureImage> textureImages)
		: m_MaterialBuffer(materialBuffer)
		, m_TextureImages(std::move(textureImages))
	{
	}
};

#endif //NEON_COMPONENTS_H
