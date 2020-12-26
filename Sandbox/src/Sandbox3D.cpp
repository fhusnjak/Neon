#include "neopch.h"

#include "Sandbox3D.h"

#include "imgui/imgui.h"

#define AVG_FRAME_COUNT 1000

namespace Neon
{
	Sandbox3D::Sandbox3D()
		: Layer("Sandbox3D")
	{
	}

	void Sandbox3D::OnAttach()
	{
	}

	void Sandbox3D::OnDetach()
	{
	}

	void Sandbox3D::OnUpdate(float ts)
	{
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
		ImGui::Begin("Frame time");
		ImGui::Text("Frame Time: %.2fms\n", m_TimePassed / AVG_FRAME_COUNT);
		ImGui::End();
	}

	void Sandbox3D::OnEvent(Event& e)
	{
	}
}
