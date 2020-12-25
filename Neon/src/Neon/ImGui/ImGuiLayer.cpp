#include "neopch.h"

#include "ImGuiLayer.h"
#include "Neon/Renderer/RendererAPI.h"
#include "Platform/Vulkan/VulkanImGuiLayer.h"

namespace Neon
{
	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::API::None:
				return nullptr;
			case RendererAPI::API::Vulkan:
				return new VulkanImGuiLayer();
		}

		NEO_CORE_ASSERT(false, "Unknown RendererAPI is selected");
		return nullptr;
	}
} // namespace Neon

/*
void Neon::ImGuiLayer::OnAttach()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(Application::Get().GetWindow().GetNativeWindow(), true);
}

void Neon::ImGuiLayer::OnDetach()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Neon::ImGuiLayer::Begin()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Neon::ImGuiLayer::End()
{
	ImGuiIO& io = ImGui::GetIO();
	const Application& app = Application::Get();
	io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());
	ImGui::Render();
	VulkanRenderer::DrawImGui();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}*/
