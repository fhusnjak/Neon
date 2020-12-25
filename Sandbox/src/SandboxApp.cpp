#include "Sandbox3D.h"

#include <Neon/Core/EntryPoint.h>

class SandboxApp : public Neon::Application
{
public:
	SandboxApp()
	{
		PushLayer(new Neon::Sandbox3D());
	}

	~SandboxApp() = default;
};

Neon::Application* Neon::CreateApplication()
{
	return new SandboxApp();
}
