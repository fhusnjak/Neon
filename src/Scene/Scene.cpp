//
// Created by Filip on 3.8.2020..
//

#include "Scene.h"
#include "VulkanRenderer.h"
#include <Renderer/Context.h>

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

void Neon::Scene::LoadSkyDome()
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		"models/dome.obj", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
							   aiProcess_JoinIdenticalVertices);
	assert(scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode);
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	ProcessNode(scene, scene->mRootNode, glm::mat4(1.0), vertices, indices);
	Entity entity = CreateEntity(scene->mRootNode->mName.C_Str());
	auto& skyDomeRenderer = entity.AddComponent<SkyDomeRenderer>();
	auto& transformComponent = entity.AddComponent<Transform>(glm::mat4(1.0));
	transformComponent.m_Transform = glm::scale(transformComponent.m_Transform, {5000, 5000, 5000});

	auto cmdBuff = VulkanRenderer::BeginSingleTimeCommands();

	skyDomeRenderer.m_Mesh.m_VerticesCount = (uint32_t)vertices.size();
	skyDomeRenderer.m_Mesh.m_IndicesCount = (uint32_t)indices.size();
	skyDomeRenderer.m_Mesh.m_VertexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, vertices,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	skyDomeRenderer.m_Mesh.m_IndexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, indices,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	Neon::VulkanRenderer::EndSingleTimeCommands(cmdBuff);

	Neon::Allocator::FlushStaging();

	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	auto& pipeline = skyDomeRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/build/vert_skydome.spv");
	pipeline.LoadFragmentShader("src/Shaders/build/frag_skydome.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
												   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({}, {pushConstantRange});
	pipeline.CreatePipeline(VulkanRenderer::GetOffscreenRenderPass(),
							VulkanRenderer::GetMsaaSamples(), VulkanRenderer::GetExtent2D(),
							{Vertex::getBindingDescription()}, {Vertex::getAttributeDescriptions()},
							vk::CullModeFlagBits::eNone);
}

void Neon::Scene::LoadModel(const std::string& filename, const glm::mat4& worldTransform)
{
	Assimp::Importer importer;
	const aiScene* scene =
		importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
										aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	assert(scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode);
	ProcessNode(scene, scene->mRootNode, glm::mat4(1.0f), worldTransform);
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
	std::vector<TextureImage> textureImages;
	std::unordered_map<std::string, uint32_t> boneMap;
	std::vector<glm::mat4> boneOffsets;

	ProcessNode(scene, scene->mRootNode, glm::mat4(1.0), vertices, indices, materials,
				textureImages, boneMap, boneOffsets);

	Entity entity = CreateEntity(scene->mRootNode->mName.C_Str());
	auto& skinnedMeshRenderer =
		entity.AddComponent<SkinnedMeshRenderer>(scene, 0, boneMap, boneOffsets);
	skinnedMeshRenderer.m_TextureImages = std::move(textureImages);
	auto& transformComponent = entity.AddComponent<Transform>(glm::mat4(1.0));

	auto cmdBuff = VulkanRenderer::BeginSingleTimeCommands();

	skinnedMeshRenderer.m_BoneBuffer = Neon::Allocator::CreateBuffer(
		sizeof(boneOffsets[0]) * skinnedMeshRenderer.m_BoneSize,
		vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

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

	Neon::Allocator::FlushStaging();

	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler,
						  static_cast<uint32_t>(skinnedMeshRenderer.m_TextureImages.size()),
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(2, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eVertex);

	vk::DescriptorBufferInfo materialBufferInfo{skinnedMeshRenderer.m_MaterialBuffer->m_Buffer, 0,
												VK_WHOLE_SIZE};

	std::vector<vk::DescriptorImageInfo> texturesBufferInfo;
	texturesBufferInfo.reserve(skinnedMeshRenderer.m_TextureImages.size());
	for (auto& textureImage : skinnedMeshRenderer.m_TextureImages)
	{
		texturesBufferInfo.push_back(textureImage.m_Descriptor);
	}

	skinnedMeshRenderer.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	vk::DescriptorBufferInfo boneBufferInfo{skinnedMeshRenderer.m_BoneBuffer->m_Buffer, 0,
											VK_WHOLE_SIZE};
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = skinnedMeshRenderer.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.Create(VulkanRenderer::GetDescriptorPool(), bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &materialBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(1, texturesBufferInfo.data(), 0),
			wavefrontDescriptorSet.CreateWrite(2, &boneBufferInfo, 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}
	auto& pipeline = skinnedMeshRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/build/animation_vert.spv");
	pipeline.LoadFragmentShader("src/Shaders/build/frag.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
												   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({skinnedMeshRenderer.m_DescriptorSets[0].GetLayout()},
								  {pushConstantRange});
	pipeline.CreatePipeline(VulkanRenderer::GetOffscreenRenderPass(),
							VulkanRenderer::GetMsaaSamples(), VulkanRenderer::GetExtent2D(),
							{Vertex::getBindingDescription()}, {Vertex::getAttributeDescriptions()},
							vk::CullModeFlagBits::eBack);

	return entity;
}

void Neon::Scene::ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 parentTransform,
							  std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
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
		vertices.push_back(vertex);
	}
}

void Neon::Scene::ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 parentTransform,
							  const glm::mat4& worldTransform)
{
	std::vector<uint32_t> indices;
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		// FIXME: for now just ignore faces with number of indices not equal to 3
		aiFace face = mesh->mFaces[i];
		if (face.mNumIndices != 3) { continue; }
		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	if (indices.empty()) { return; }

	std::vector<Vertex> vertices;
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex{};
		vertex.pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
		vertex.norm = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
		vertex.matID = 0;
		// TODO: for now just use first texture coordinate
		if (mesh->mTextureCoords[0])
		{ vertex.texCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}; }
		else
		{
			vertex.texCoord = {0.0f, 0.0f};
		}
		vertices.push_back(vertex);
	}

	Entity entity = CreateEntity(mesh->mName.C_Str());
	auto& water = entity.AddComponent<Water>(VulkanRenderer::GetExtent2D());
	auto& transform = entity.AddComponent<Transform>(worldTransform * parentTransform);

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

	/*std::unique_ptr<ImageAllocation> imageAllocation;
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
		imageAllocation->m_Image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	vk::SamplerCreateInfo samplerInfo = {
		{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
	samplerInfo.setMaxLod(FLT_MAX);
	vk::Sampler sampler = Neon::VulkanRenderer::CreateSampler(samplerInfo);
	vk::DescriptorImageInfo desc{sampler, textureImageView,
								 vk::ImageLayout::eShaderReadOnlyOptimal};*/

	//water.m_TextureImages.emplace_back(desc, std::move(imageAllocation));

	auto cmdBuff = VulkanRenderer::BeginSingleTimeCommands();

	water.m_Mesh.m_VerticesCount = (uint32_t)vertices.size();
	water.m_Mesh.m_IndicesCount = (uint32_t)indices.size();
	water.m_Mesh.m_VertexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, vertices,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	water.m_Mesh.m_IndexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, indices,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	water.m_MaterialBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, materials, vk::BufferUsageFlagBits::eStorageBuffer);

	VulkanRenderer::EndSingleTimeCommands(cmdBuff);

	Allocator::FlushStaging();

	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler,
						  static_cast<uint32_t>(water.m_TextureImages.size()),
						  vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorBufferInfo materialBufferInfo{water.m_MaterialBuffer->m_Buffer, 0, VK_WHOLE_SIZE};

	std::vector<vk::DescriptorImageInfo> texturesBufferInfo;
	texturesBufferInfo.reserve(water.m_TextureImages.size());
	for (auto& texture : water.m_TextureImages)
	{
		texturesBufferInfo.push_back(texture.m_Descriptor);
	}

	water.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = water.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.Create(VulkanRenderer::GetDescriptorPool(), bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &materialBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(1, texturesBufferInfo.data(), 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}
	auto& pipeline = water.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/build/vert_water.spv");
	pipeline.LoadFragmentShader("src/Shaders/build/frag_water.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
												   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({water.m_DescriptorSets[0].GetLayout()}, {pushConstantRange});
	pipeline.CreatePipeline(VulkanRenderer::GetOffscreenRenderPass(),
							VulkanRenderer::GetMsaaSamples(), VulkanRenderer::GetExtent2D(),
							{Vertex::getBindingDescription()}, {Vertex::getAttributeDescriptions()},
							vk::CullModeFlagBits::eBack);
}

void Neon::Scene::ProcessMesh(const aiScene* scene, aiMesh* mesh, glm::mat4 parentTransform,
							  std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
							  std::vector<Material>& materials,
							  std::vector<TextureImage>& textureImages,
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
	//material.diffuse = {diffuseColor.r, diffuseColor.g, diffuseColor.b};
	material.diffuse = {0.8f, 0.8f, 0.8f};
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
		imageAllocation->m_Image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	vk::SamplerCreateInfo samplerInfo = {
		{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
	samplerInfo.setMaxLod(FLT_MAX);
	vk::Sampler sampler = Neon::VulkanRenderer::CreateSampler(samplerInfo);
	vk::DescriptorImageInfo desc{sampler, textureImageView,
								 vk::ImageLayout::eShaderReadOnlyOptimal};
	textureImages.emplace_back(desc, std::move(imageAllocation));
}

void Neon::Scene::OnUpdate(float ts, Neon::PerspectiveCameraController controller,
						   glm::vec4 clearColor, bool pointLight, float lightIntensity,
						   glm::vec3 lightDirection, glm::vec3 lightPosition)
{
	auto animationView = m_Registry.view<SkinnedMeshRenderer>();
	for (auto entity : animationView)
	{
		auto& skinnedMeshRenderer = animationView.get<SkinnedMeshRenderer>(entity);
		skinnedMeshRenderer.Update(ts / 1000.0f);
	}

	auto camera = controller.GetCamera();
	float translation = -((camera.GetPosition().y + 5) * 2);
	camera.Translate({0, translation, 0});
	camera.InvertPitch();

	auto waterGroup = m_Registry.group<Water>(entt::get<Transform>);
	for (auto entity : waterGroup)
	{
		const auto& [water, transform] = waterGroup.get<Water, Transform>(entity);
		VulkanRenderer::BeginScene(water.m_FrameBuffers, clearColor, camera, {0, 1, 0, 5},
								   pointLight, lightIntensity, lightDirection, lightPosition);
		Render(camera);
		VulkanRenderer::EndScene();
	}

	camera.Translate({0, -translation, 0});
	camera.InvertPitch();

	VulkanRenderer::BeginScene(VulkanRenderer::GetOffscreenFramebuffers(), clearColor, camera,
							   {0, 1, 0, 100000}, pointLight, lightIntensity, lightDirection,
							   lightPosition);
	Render(camera);
	for (auto entity : waterGroup)
	{
		const auto& [water, transform] = waterGroup.get<Water, Transform>(entity);
		VulkanRenderer::Render(transform, water);
	}
	VulkanRenderer::EndScene();
}

float GetHeight(unsigned char* pixels, int texWidth, int texHeight, int texX, int texY,
				float maxHeight)
{
	if (texX < 0 || texY < 0 || texX >= texWidth || texY >= texHeight) { return 0.0f; }
	unsigned char* valuePtr = pixels + static_cast<int>((texY * texWidth + texX) * 4);
	uint32_t value =
		(uint32_t)*valuePtr + (uint32_t) * (valuePtr + 1) + (uint32_t) * (valuePtr + 2);
	return static_cast<float>(value) / 765.0f * maxHeight;
}

glm::vec3 CalculateNormal(unsigned char* pixels, int texWidth, int texHeight, int texX, int texY,
						  float maxHeight)
{
	float heightL = GetHeight(pixels, texWidth, texHeight, texX + 1, texY, maxHeight);
	float heightR = GetHeight(pixels, texWidth, texHeight, texX - 1, texY, maxHeight);
	float heightD = GetHeight(pixels, texWidth, texHeight, texX, texY - 1, maxHeight);
	float heightU = GetHeight(pixels, texWidth, texHeight, texX, texY + 1, maxHeight);
	glm::vec3 normal = {heightR - heightL, 2.0f, heightD - heightU};
	return glm::normalize(normal);
}

void CreateTextureImage(const std::string& filename, Neon::TextureImage& textureImage)
{
	textureImage.m_TextureAllocation = Neon::Allocator::CreateTextureImage(filename);
	assert(textureImage.m_TextureAllocation);

	vk::ImageView textureImageView = Neon::VulkanRenderer::CreateImageView(
		textureImage.m_TextureAllocation->m_Image, vk::Format::eR8G8B8A8Srgb,
		vk::ImageAspectFlagBits::eColor);
	vk::SamplerCreateInfo samplerInfo = {
		{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear};
	samplerInfo.setMaxLod(FLT_MAX);
	vk::Sampler sampler = Neon::VulkanRenderer::CreateSampler(samplerInfo);
	textureImage.m_Descriptor = {sampler, textureImageView,
								 vk::ImageLayout::eShaderReadOnlyOptimal};
}

struct VertexTerrain
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 color;
	glm::vec2 mapTexCoord;
	glm::vec2 tileTexCoord;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return {0, sizeof(VertexTerrain)};
	}

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<vk::VertexInputAttributeDescription> result = {
			{0, 0, vk::Format::eR32G32B32Sfloat,
			 static_cast<uint32_t>(offsetof(VertexTerrain, pos))},
			{1, 0, vk::Format::eR32G32B32Sfloat,
			 static_cast<uint32_t>(offsetof(VertexTerrain, norm))},
			{2, 0, vk::Format::eR32G32B32Sfloat,
			 static_cast<uint32_t>(offsetof(VertexTerrain, color))},
			{3, 0, vk::Format::eR32G32Sfloat,
			 static_cast<uint32_t>(offsetof(VertexTerrain, mapTexCoord))},
			{4, 0, vk::Format::eR32G32Sfloat,
			 static_cast<uint32_t>(offsetof(VertexTerrain, tileTexCoord))}};
		return result;
	}

	bool operator==(const VertexTerrain& other) const
	{
		return pos == other.pos;
	}
};

void Neon::Scene::LoadTerrain(float width, float height, float maxHeight)
{
	std::vector<VertexTerrain> vertices;
	std::vector<uint32_t> indices;

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels =
		stbi_load("textures/heightmap.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	assert(pixels);

	float densityX = static_cast<float>(texWidth) / (width * 2.0f + 1);
	float densityY = static_cast<float>(texHeight) / (height * 2.0f + 1);

	int widthCount = static_cast<int>(densityX * (width * 2.0f + 1));
	int heightCount = static_cast<int>(densityY * (height * 2.0f + 1));
	float widthIncrement = 1.0f / (densityX);
	float heightIncrement = 1.0f / (densityY);
	float xValue = -width;
	float zValue = -height;

	float mapTexCoordXIncrement = 1.0f / static_cast<float>(widthCount);
	float mapTexCoordYIncrement = 1.0f / static_cast<float>(heightCount);
	float mapTexCoordX = 0;
	float mapTexCoordY = 0;
	float tileTexCoordX = 0;
	float tileTexCoordY = 0;
	for (int i = 0; i < heightCount; i++)
	{
		for (int j = 0; j < widthCount; j++)
		{
			VertexTerrain vertex{};
			int texX = static_cast<int>(static_cast<float>(j) / static_cast<float>(widthCount) *
										static_cast<float>(texWidth));
			int texY = static_cast<int>(static_cast<float>(i) / static_cast<float>(heightCount) *
										static_cast<float>(texHeight));
			vertex.pos = {xValue, GetHeight(pixels, texWidth, texHeight, texX, texY, maxHeight),
						  zValue};
			vertex.norm = CalculateNormal(pixels, texWidth, texHeight, texX, texY, maxHeight);
			vertex.mapTexCoord = {mapTexCoordX, mapTexCoordY};
			vertex.tileTexCoord = {tileTexCoordX, tileTexCoordY};
			vertices.push_back(vertex);
			xValue += widthIncrement;
			mapTexCoordX += mapTexCoordXIncrement;
			tileTexCoordX += 0.5;
		}
		xValue = -width;
		zValue += heightIncrement;
		mapTexCoordX = 0;
		mapTexCoordY += mapTexCoordYIncrement;
		tileTexCoordX = 0;
		tileTexCoordY += 0.5f;
	}

	uint32_t index = 0;
	for (int i = 0; i < heightCount - 1; i++)
	{
		for (int j = 0; j < widthCount - 1; j++)
		{
			indices.push_back(index);
			indices.push_back(index + widthCount);
			indices.push_back(index + 1);
			indices.push_back(index + 1);
			indices.push_back(index + widthCount);
			indices.push_back(index + widthCount + 1);
			index++;
		}
		index++;
	}

	Entity entity = CreateEntity("terrain");
	auto& terrainRenderer = entity.AddComponent<TerrainRenderer>();
	auto& transform = entity.AddComponent<Transform>(glm::translate(glm::mat4(1.0), {0, -15, 0}));

	std::vector<Material> materials;
	Material material{};
	material.ambient = {0.1, 0.1, 0.1};
	material.diffuse = {0.8, 0.8, 0.8};
	material.specular = {0.0, 0.0, 0.0};
	material.textureID = 0;
	materials.push_back(material);

	CreateTextureImage("textures/blendMap.png", terrainRenderer.m_BlendMap);
	CreateTextureImage("textures/grassy2.png", terrainRenderer.m_BackgroundTexture);
	CreateTextureImage("textures/mud.png", terrainRenderer.m_RTexture);
	CreateTextureImage("textures/grassFlowers.png", terrainRenderer.m_GTexture);
	CreateTextureImage("textures/path.png", terrainRenderer.m_BTexture);

	auto cmdBuff = VulkanRenderer::BeginSingleTimeCommands();

	terrainRenderer.m_Mesh.m_VerticesCount = (uint32_t)vertices.size();
	terrainRenderer.m_Mesh.m_IndicesCount = (uint32_t)indices.size();
	terrainRenderer.m_Mesh.m_VertexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, vertices,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	terrainRenderer.m_Mesh.m_IndexBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, indices,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);
	terrainRenderer.m_MaterialBuffer = Allocator::CreateDeviceLocalBuffer(
		cmdBuff, materials, vk::BufferUsageFlagBits::eStorageBuffer);

	VulkanRenderer::EndSingleTimeCommands(cmdBuff);

	Allocator::FlushStaging();

	const auto& device = Neon::Context::GetInstance().GetLogicalDevice().GetHandle();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(3, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(4, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);
	bindings.emplace_back(5, vk::DescriptorType::eCombinedImageSampler, 1,
						  vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorBufferInfo materialBufferInfo{terrainRenderer.m_MaterialBuffer->m_Buffer, 0,
												VK_WHOLE_SIZE};

	terrainRenderer.m_DescriptorSets.resize(MAX_SWAP_CHAIN_IMAGES);
	for (int i = 0; i < MAX_SWAP_CHAIN_IMAGES; i++)
	{
		auto& wavefrontDescriptorSet = terrainRenderer.m_DescriptorSets[i];
		wavefrontDescriptorSet.Init(device);
		wavefrontDescriptorSet.Create(VulkanRenderer::GetDescriptorPool(), bindings);
		std::vector<vk::WriteDescriptorSet> descriptorWrites = {
			wavefrontDescriptorSet.CreateWrite(0, &materialBufferInfo, 0),
			wavefrontDescriptorSet.CreateWrite(1, &terrainRenderer.m_BlendMap.m_Descriptor, 0),
			wavefrontDescriptorSet.CreateWrite(2, &terrainRenderer.m_BackgroundTexture.m_Descriptor,
											   0),
			wavefrontDescriptorSet.CreateWrite(3, &terrainRenderer.m_RTexture.m_Descriptor, 0),
			wavefrontDescriptorSet.CreateWrite(4, &terrainRenderer.m_GTexture.m_Descriptor, 0),
			wavefrontDescriptorSet.CreateWrite(5, &terrainRenderer.m_BTexture.m_Descriptor, 0)};
		wavefrontDescriptorSet.Update(descriptorWrites);
	}

	auto& pipeline = terrainRenderer.m_GraphicsPipeline;
	pipeline.Init(device);
	pipeline.LoadVertexShader("src/Shaders/build/vert_terrain.spv");
	pipeline.LoadFragmentShader("src/Shaders/build/frag_terrain.spv");

	vk::PushConstantRange pushConstantRange = {vk::ShaderStageFlagBits::eVertex |
												   vk::ShaderStageFlagBits::eFragment,
											   0, sizeof(PushConstant)};
	pipeline.CreatePipelineLayout({terrainRenderer.m_DescriptorSets[0].GetLayout()},
								  {pushConstantRange});
	pipeline.CreatePipeline(
		VulkanRenderer::GetOffscreenRenderPass(), VulkanRenderer::GetMsaaSamples(),
		VulkanRenderer::GetExtent2D(), {VertexTerrain::getBindingDescription()},
		{VertexTerrain::getAttributeDescriptions()}, vk::CullModeFlagBits::eBack);
}

void Neon::Scene::Render(Neon::PerspectiveCamera camera)
{
	auto skyDomeGroup = m_Registry.group<SkyDomeRenderer>(entt::get<Transform>);
	for (auto entity : skyDomeGroup)
	{
		auto [skyDomeRenderer, transform] = skyDomeGroup.get<SkyDomeRenderer, Transform>(entity);
		glm::mat4 transformMatrix = transform.m_Transform;
		transformMatrix =
			glm::translate(glm::mat4(1.0), camera.GetPosition()) * transform.m_Transform;
		transformMatrix = glm::translate(glm::mat4(1.0), {0, -1000, 0}) * transform.m_Transform;
		Transform newTransform(transformMatrix);
		VulkanRenderer::Render(newTransform, skyDomeRenderer);
	}
	auto terrainGroup = m_Registry.group<TerrainRenderer>(entt::get<Transform>);
	for (auto entity : terrainGroup)
	{
		const auto& [terrainRenderer, transform] =
			terrainGroup.get<TerrainRenderer, Transform>(entity);
		VulkanRenderer::Render(transform, terrainRenderer);
	}
	auto animationGroup = m_Registry.group<SkinnedMeshRenderer>(entt::get<Transform>);
	for (auto entity : animationGroup)
	{
		const auto& [skinnedMeshRenderer, transform] =
			animationGroup.get<SkinnedMeshRenderer, Transform>(entity);
		VulkanRenderer::Render(transform, skinnedMeshRenderer);
	}
}
