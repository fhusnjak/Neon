#include "neopch.h"

#include "Application.h"
#include "Neon/ImGui/ImGuiLayer.h"
#include "Neon/Renderer/Renderer.h"

#include <imgui/imgui.h>

namespace Neon
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationProps& applicationProps)
	{
		NEO_ASSERT(s_Instance == nullptr, "Application already exists");
		s_Instance = this;

		m_Window = UniquePtr<Window>(
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
				ImGui::Begin("Renderer");
				ImGui::Text("Vendor: %s", "Vulkan");
				ImGui::Text("Renderer: %s", "Vulkan");
				ImGui::Text("Version: %s", "1.0");
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
		m_Minimized = false;
		return false;
	}
} // namespace Neon
