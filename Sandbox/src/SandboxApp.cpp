#include "Sandbox3D.h"

#include <Core/EntryPoint.h>

class Sandbox : public Neon::Application
{
public:
	Sandbox()
	{
		PushLayer(new Sandbox3D());
	}

	~Sandbox() = default;
};

Neon::Application* Neon::CreateApplication()
{
	return new Sandbox();
}
