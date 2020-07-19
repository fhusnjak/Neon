#include "vkpch.h"

#include "Sandbox3D.h"

#include "Renderer/VulkanRenderer.h"

#include "glm/gtc/matrix_inverse.hpp"

#include <examples/imgui_impl_glfw.h>

#define AVG_FRAME_COUNT 100

bool hdr;

Sandbox3D::Sandbox3D()
	: Layer("Sandbox3D")
	, m_CameraController((float)WIDTH / HEIGHT)
{
	ObjModel::LoadSkysphere();

	m_Models.push_back(ObjModel::LoadModel("models/Sphere.obj"));
	m_Models.push_back(ObjModel::LoadModel("models/plane.obj"));
	m_Models.push_back(ObjModel::LoadModel("models/wuson.obj"));

	VulkanRenderer::PushModel(m_Models[0]);
	VulkanRenderer::PushModel(m_Models[1]);
	VulkanRenderer::PushModel(m_Models[2]);

	auto model = glm::translate(glm::mat4(1.0), glm::vec3(-5.0, 0.0, 0.0));
	m_Instances.push_back({0, model, glm::inverseTranspose(model), m_Models[0].textureOffset});
	model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, -1.1, 0.0));
	m_Instances.push_back({1, model, glm::inverseTranspose(model), m_Models[1].textureOffset});
	model = glm::translate(glm::mat4(1.0), glm::vec3(-2.0, -1.0, 0.0));
	m_Instances.push_back({2, model, glm::inverseTranspose(model), m_Models[2].textureOffset});

	VulkanRenderer::Flush(m_Instances);
}

void Sandbox3D::OnAttach() { }

void Sandbox3D::OnDetach() { }

void Sandbox3D::OnUpdate(float ts)
{
	m_CameraController.OnUpdate(ts);

	VulkanRenderer::BeginScene(m_CameraController.GetCamera());
	VulkanRenderer::Rasterize(clearColor, lightPosition);
	VulkanRenderer::EndScene();

	m_Times.push(ts);
	m_TimePassed += ts;
	m_FrameCount++;
	if (m_FrameCount > AVG_FRAME_COUNT)
	{
		m_FrameCount = AVG_FRAME_COUNT;
		m_TimePassed -= m_Times.front();
		m_Times.pop();
	}
}

void Sandbox3D::OnImGuiRender()
{
	ImGui::Text("FPS %.0f", 1000.0f * static_cast<float>(m_FrameCount) / m_TimePassed);
	ImGui::SliderFloat3("Light Position", &lightPosition.x, -20.f, 20.f);
	ImGui::Checkbox("HDR", &hdr);
}

void Sandbox3D::OnEvent(Event& e)
{
	m_CameraController.OnEvent(e);
}
