#include "neopch.h"

#include "Application.h"

int main()
{
	Application* app = new Application("Neon");
	app->Run();
	delete app;
	return 0;
}