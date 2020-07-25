#pragma once

#include "Core/ImGuiLayer.h"
#include "Core/LayerStack.h"
#include "Event/ApplicationEvent.h"
#include "Event/Event.h"
#include "Window/Window.h"

#include <chrono>

class Application
{
public:
	explicit Application(const std::string& name) noexcept;
	~Application();
	Application(const Application& other) = delete;
	Application& operator=(const Application& other) = delete;
	Application(const Application&& other) = delete;
	Application& operator=(const Application&& other) = delete;

	void Run();
	void OnEvent(Event& e);
	void PushLayer(Layer* layer);
	void PushOverlay(Layer* layer);

	[[nodiscard]] const inline Window& GetWindow() const noexcept
	{
		return m_Window;
	}

	static const Application& Get() noexcept
	{
		return *s_Instance;
	}

private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

private:
	static Application* s_Instance;

	Window m_Window;
	bool m_Running = true;
	bool m_Minimized = false;
	ImGuiLayer* m_ImGuiLayer;
	LayerStack m_LayerStack;
	std::chrono::time_point<std::chrono::steady_clock> m_LastFrameTime =
		std::chrono::high_resolution_clock::now();
};