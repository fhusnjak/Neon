#pragma once

#include "Core/Allocator.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct ObjInstance
{
	uint32_t objModelIndex{0};
	glm::mat4 modelMatrix{1};
	glm::mat4 modelMatrixIT{1};
	uint32_t textureOffset{};
};

struct ObjModel
{
public:
	ObjModel() = default;
	static void LoadSkysphere();
	static void LoadHdrSkysphere();
	static std::vector<TextureImage> s_SkyboxTextureImages;
	static TextureImage s_Skysphere;
	static TextureImage s_HdrSkysphere;

public:
};