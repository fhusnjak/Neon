#include "neopch.h"

#include "Layer.h"

#include <utility>

Neon::Layer::Layer(std::string name)
	: m_DebugName(std::move(name))
{
}
