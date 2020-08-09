//
// Created by Filip on 3.8.2020..
//

#include "Entity.h"

Neon::Entity::Entity(entt::entity handle, Scene* scene)
	: m_EntityHandle(handle)
	, m_Scene(scene)
{
}
