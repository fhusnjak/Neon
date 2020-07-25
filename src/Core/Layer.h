#pragma once

#include "Event/Event.h"

class Layer
{
public:
	explicit Layer(std::string  name = "Layer");
	virtual ~Layer() = default;

	virtual void OnAttach() { }

	virtual void OnDetach() { }

	virtual void OnUpdate(float ts) { }

	virtual void OnImGuiRender() { }

	virtual void OnEvent(Event& event) { }

	[[nodiscard]] inline const std::string& GetName() const
	{
		return m_DebugName;
	}

protected:
	std::string m_DebugName;
};
