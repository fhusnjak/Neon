#pragma once

#include "Core/Layer.h"

class ImGuiLayer : public Layer
{
public:
	ImGuiLayer() = default;
	~ImGuiLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;

	static void Begin();
	static void End();
};