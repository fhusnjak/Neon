//
// Created by Filip on 3.8.2020..
//

#ifndef NEON_ENTITY_H
#define NEON_ENTITY_H

#include "Scene.h"
#include "entt.h"

class Entity
{
public:
	Entity() = default;
	Entity(entt::entity handle, Scene* scene);
	Entity(const Entity& other) = default;

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		assert(!HasComponent<T>());
		return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template<typename T>
	T& GetComponent()
	{
		assert(HasComponent<T>());
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	bool HasComponent()
	{
		return m_Scene->m_Registry.has<T>(m_EntityHandle);
	}

	template<typename T>
	void RemoveComponent()
	{
		assert(HasComponent<T>());
		m_Scene->m_Registry.remove<T>(m_EntityHandle);
	}

	operator bool() const
	{
		return m_EntityHandle != entt::null;
	}

private:
	entt::entity m_EntityHandle{entt::null};
	Scene* m_Scene = nullptr;
};

#endif //NEON_ENTITY_H
