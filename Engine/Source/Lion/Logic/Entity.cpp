#include "Engine.h"
#include "Entity.h"

#include <Lion/Logic/Scene.h>

namespace Lion
{
	Entity::Entity()
		: mTransform(MakeReference<Transform>())
	{
	}

	void Entity::RemoveFromScene()
	{
		if (mScene)
			mScene->Remove(this);
	}

	void Entity::RegisterComponent(Scope<Component> component, std::type_index type)
	{
		component->mOwner = this;

		Component* raw = component.get();
		mComponents.push_back(std::move(component));
		mComponentLookup.insert_or_assign(type, raw);

		raw->OnAttach();
	}

	Component* Entity::FindComponent(std::type_index type) const
	{
		const auto it = mComponentLookup.find(type);
		return (it != mComponentLookup.end()) ? it->second : nullptr;
	}

	void Entity::UnregisterComponent(std::type_index type)
	{
		const auto it = mComponentLookup.find(type);

		if (it == mComponentLookup.end())
			return;

		Component* target = it->second;
		target->OnDestroy();
		mComponentLookup.erase(it);

		mComponents.erase(
			std::remove_if(mComponents.begin(), mComponents.end(),
				[target](const Scope<Component>& component) { return component.get() == target; }),
			mComponents.end());
	}

	void Entity::Awake()
	{
		OnAwake();

		for (const auto& component : mComponents)
			component->OnAwake();
	}

	void Entity::Destroy()
	{
		for (const auto& component : mComponents)
			component->OnDestroy();

		OnDestroy();
	}

	void Entity::UpdateBegin()
	{
		OnUpdateBegin();

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnUpdateBegin();
	}

	void Entity::Update()
	{
		OnUpdate();

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnUpdate();
	}

	void Entity::UpdateEnd()
	{
		OnUpdateEnd();

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnUpdateEnd();
	}

	void Entity::Render()
	{
		OnRender();

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnRender();
	}
}
