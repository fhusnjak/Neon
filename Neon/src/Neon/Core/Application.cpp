#include "neopch.h"

#include "Application.h"
#include "Neon/ImGui/ImGuiLayer.h"
#include "Neon/Renderer/Renderer.h"
#include "Neon/Renderer/Framebuffer.h"

#include <imgui/imgui.h>

namespace Neon
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationProps& applicationProps)
	{
		NEO_ASSERT(s_Instance == nullptr, "Application already exists");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(
			Window::Create(WindowProps{applicationProps.Name, applicationProps.WindowWidth, applicationProps.WindowHeight}));
		m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
		m_Window->SetVSync(false);

		m_ImGuiLayer = ImGuiLayer::Create();
		PushOverlay(m_ImGuiLayer);

		Renderer::Init();
	}

	Application::~Application()
	{
		Renderer::Shutdown();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled)
			{
				break;
			}
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			auto time = std::chrono::high_resolution_clock::now();
			auto timeStep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			float timeStepMilis = std::chrono::duration<float, std::chrono::milliseconds::period>(timeStep).count();

			m_Window->ProcessEvents();

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(timeStepMilis);
				}

				m_Window->GetRenderContext()->BeginFrame();

				m_ImGuiLayer->Begin();

				static bool dockSpaceOpen = true;
				static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

				ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
				ImGuiViewport* imGuiViewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(imGuiViewport->GetWorkPos());
				ImGui::SetNextWindowSize(imGuiViewport->GetWorkSize());
				ImGui::SetNextWindowViewport(imGuiViewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |=
					ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

				// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
				// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
				// all active windows docked into it will lose their parent and become undocked.
				// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
				// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::Begin("DockSpace Demo", &dockSpaceOpen, window_flags);
				ImGui::PopStyleVar();

				ImGui::PopStyleVar(2);

				// DockSpace
				ImGuiIO& io = ImGui::GetIO();
				if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
				{
					ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
					ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
				}

				RendererAPI::RenderAPICapabilities& caps = RendererAPI::GetCapabilities();
				ImGui::Begin("Renderer");
				ImGui::Text("Vendor: %s", caps.Vendor.c_str());
				ImGui::Text("Renderer: %s", caps.Renderer.c_str());
				ImGui::Text("Version: %s", caps.Version.c_str());
				ImGui::Text("Frame Time: %.2fms\n", timeStepMilis);
				ImGui::End();

				for (Layer* layer : m_LayerStack)
				{
					layer->OnImGuiRender();
				}

				Renderer::Render();

				m_ImGuiLayer->End();

				m_Window->SwapBuffers();
			}
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Window->GetRenderContext()->OnResize(e.GetWidth(), e.GetHeight());

		auto& fbs = FramebufferPool::Get().GetAll();
		for (auto& fb : fbs)
		{
			fb->Resize(e.GetWidth(), e.GetHeight());
		}

		m_Minimized = false;
		return false;
	}
} // namespace Neon
