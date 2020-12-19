#pragma once

#include "neopch.h"

#include "Application.h"

extern Neon::Application* Neon::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Neon::CreateApplication();
	app->Run();
	delete app;
	return 0;
}
