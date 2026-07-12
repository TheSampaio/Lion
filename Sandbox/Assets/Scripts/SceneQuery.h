#pragma once

#include <Lion/Lion.h>

// Reaching another entity's traits.
//
// A component is not a type another component can hold a pointer to across entities — the scene owns
// the entities, and an entity is only ever the sum of what is attached to it. So a component that
// needs to talk to another one asks the scene for it by type, which is also what keeps the wiring
// honest: nothing here knows how the scene was built.

template<typename T>
T* FindInScene(const Lion::Reference<Lion::Scene>& scene)
{
	for (const auto& entity : scene->GetEntities())
		if (T* found = entity->GetComponent<T>())
			return found;

	return nullptr;
}

template<typename T>
Lion::int32 CountInScene(const Lion::Reference<Lion::Scene>& scene)
{
	Lion::int32 count = 0;

	for (const auto& entity : scene->GetEntities())
		if (entity->HasComponent<T>())
			count++;

	return count;
}
