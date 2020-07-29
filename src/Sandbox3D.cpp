#include "neopch.h"

#include "Sandbox3D.h"

#include "Renderer/VulkanRenderer.h"

#include "glm/gtc/matrix_inverse.hpp"

#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

#define AVG_FRAME_COUNT 100

Sandbox3D::Sandbox3D()
	: Layer("Sandbox3D")
	, m_CameraController((float)WIDTH / HEIGHT)
{
	ObjModel::LoadSkysphere();

	m_Models.push_back(ObjModel::LoadModel("models/Sphere.obj"));
	m_Models.push_back(ObjModel::LoadModel("models/plane.obj"));
	m_Models.push_back(ObjModel::LoadModel("models/wuson.obj"));

	auto model = glm::translate(glm::mat4(1.0), glm::vec3(-5.0, 0.0, 0.0));
	m_Instances.push_back({0, model, glm::inverseTranspose(model), m_Models[0].textureOffset});
	model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, -1.1, 0.0));
	m_Instances.push_back({1, model, glm::inverseTranspose(model), m_Models[1].textureOffset});
	model = glm::translate(glm::mat4(1.0), glm::vec3(-2.0, -1.0, 0.0));
	m_Instances.push_back({2, model, glm::inverseTranspose(model), m_Models[2].textureOffset});

	VulkanRenderer::PushModels(m_Models);
	VulkanRenderer::PushInstances(m_Instances);
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
	static bool dockSpaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
						ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockSpaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen) ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	ImGui::Begin("Settings");
	ImGui::Text("FPS %.0f", 1000.0f * m_FrameCount / m_TimePassed);
	vk::Extent2D extent = VulkanRenderer::GetExtent2D();
	ImGui::Image(VulkanRenderer::GetOffscreenImageID(),
				 ImVec2{(float)extent.width, (float)extent.height});
	ImGui::End();

	ImGui::End();
}

void Sandbox3D::OnEvent(Event& e)
{
	m_CameraController.OnEvent(e);
}
