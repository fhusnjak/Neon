//
// Created by Filip on 3.8.2020..
//

#ifndef NEON_SCENE_H
#define NEON_SCENE_H

#include "Core/Allocator.h"

#include "PerspectiveCameraController.h"
#include "entt.h"

namespace Neon
{
class Entity;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 color;
	glm::vec2 texCoord;
	int matID;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return {0, sizeof(Vertex)};
	}

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
	{
		return {
			{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
			{1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, norm))},
			{2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, color))},
			{3, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, texCoord))},
			{4, 0, vk::Format::eR32Sint, static_cast<uint32_t>(offsetof(Vertex, matID))}};
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos;
	}
};

struct Material
{
	glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	glm::vec3 diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
	glm::vec3 specular = glm::vec3(0.2f, 0.2f, 0.2f);
	glm::vec3 transmittance = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 emission = glm::vec3(0.0f, 0.0f, 0.1f);
	float shininess = 0.f;
	float ior = 1.0f; // index of refraction
	float dissolve = 1.f; // 1 == opaque; 0 == fully transparent
	int illum = 1;
	int textureID = 0;
};

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	Entity CreateEntity(const std::string& name = std::string());

	Entity CreateWavefrontEntity(const std::string& filename,
								 const std::string& name = std::string());

	void OnUpdate(float ts, Neon::PerspectiveCameraController controller, glm::vec4 clearColor,
				  glm::vec3 lightPosition);

private:
	entt::registry m_Registry;

	friend class Entity;
};
} // namespace Neon

#endif //NEON_SCENE_H
