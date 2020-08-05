#include "neopch.h"

#include "Renderer/VulkanRenderer.h"

#include "Application.h"

#include "Sandbox3D.h"

Application* Application::s_Instance = nullptr;

Application::Application(const std::string& name) noexcept
	: m_Window(name)
{
	assert(s_Instance == nullptr);
	s_Instance = this;
	m_Window.SetEventCallback([this](Event& e) { OnEvent(e); });
	m_ImGuiLayer = new ImGuiLayer();
	PushOverlay(m_ImGuiLayer);
	VulkanRenderer::Init(&m_Window);
	PushLayer(new Sandbox3D());
}

Application::~Application()
{
	VulkanRenderer::Shutdown();
}

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
	dispatcher.Dispatch<WindowResizeEvent>(
		[this](WindowResizeEvent& e) { return OnWindowResize(e); });

	for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
	{
		(*it)->OnEvent(e);
		if (e.Handled) break;
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

		m_Window.OnUpdate();

		VulkanRenderer::Begin();
		if (!m_Minimized)
		{
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(
					std::chrono::duration<float, std::chrono::milliseconds::period>(timeStep)
						.count());
			ImGuiLayer::Begin();
			{
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			ImGuiLayer::End();
		}
		VulkanRenderer::End();
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
	m_Minimized = false;
	return false;
}
