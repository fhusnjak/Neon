//
// Created by Filip on 3.8.2020..
//

#ifndef NEON_SCENE_H
#define NEON_SCENE_H

#include "Core/Allocator.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "PerspectiveCameraController.h"
#include "entt.h"

#define MAX_BONES_PER_VERTEX 10

namespace Neon
{
class Entity;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 color;
	glm::vec2 texCoord;
	uint32_t matID;
	uint32_t boneIDs[MAX_BONES_PER_VERTEX];
	float boneWeights[MAX_BONES_PER_VERTEX];

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return {0, sizeof(Vertex)};
	}

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<vk::VertexInputAttributeDescription> result = {
			{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
			{1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, norm))},
			{2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, color))},
			{3, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, texCoord))},
			{4, 0, vk::Format::eR32Sint, static_cast<uint32_t>(offsetof(Vertex, matID))}};
		for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++)
		{
			result.emplace_back(
				i + 5, 0, vk::Format::eR32Uint,
				static_cast<uint32_t>(offsetof(Vertex, boneIDs) + i * sizeof(uint32_t)));
		}
		for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++)
		{
			result.emplace_back(
				i + 5 + MAX_BONES_PER_VERTEX, 0, vk::Format::eR32Sfloat,
				static_cast<uint32_t>(offsetof(Vertex, boneWeights) + i * sizeof(float)));
		}
		return result;
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
	float shininess = 1.f;
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

	void LoadSkyDome();

	void LoadModel(const std::string& filename, const glm::mat4& worldTransform);

	void LoadTerrain(float width, float height, float maxHeight);

	Entity LoadAnimatedModel(const std::string& filename);

	void OnUpdate(float ts, Neon::PerspectiveCameraController controller, glm::vec4 clearColor,
				  bool pointLight, float lightIntensity, glm::vec3 lightDirection,
				  glm::vec3 lightPosition);

private:
	template<typename... Args>
	void ProcessNode(const aiScene* scene, aiNode* node, glm::mat4 parentTransform, Args&&... args)
	{
		glm::mat4 nodeTransform =
			parentTransform * glm::transpose(*(glm::mat4*)&node->mTransformation);
		for (int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(scene, mesh, nodeTransform, std::forward<Args>(args)...);
		}
		for (int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(scene, node->mChildren[i], nodeTransform, std::forward<Args>(args)...);
		}
	}
	static void ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 parentTransform,
							std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	void ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 parentTransform,
					 const glm::mat4& worldTransform);
	static void ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 parentTransform,
							std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
							std::vector<Material>& materials,
							std::vector<std::shared_ptr<TextureImage>>& textureImages,
							std::unordered_map<std::string, uint32_t>& boneMap,
							std::vector<glm::mat4>& boneOffsets);

private:
	entt::registry m_Registry;
	friend class Entity;
};
} // namespace Neon

#endif //NEON_SCENE_H
