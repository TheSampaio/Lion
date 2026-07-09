#include "Engine.h"
#include "Entity.h"

#include <Lion/Logic/Scene.h>

namespace Lion
{
	// Monotonic source of process-unique entity ids. 0 is reserved to mean "no entity", so ids
	// start at 1 (the framebuffer's entity-id attachment is cleared to -1 for empty pixels).
	static int32 sNextEntityId = 1;

	Entity::Entity()
		: mId(sNextEntityId++), mTransform(MakeReference<Transform>())
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

	void Entity::RemoveComponent(Component* component)
	{
		if (!component)
			return;

		for (auto it = mComponentLookup.begin(); it != mComponentLookup.end(); ++it)
		{
			if (it->second == component)
			{
				mComponentLookup.erase(it);
				break;
			}
		}

		component->OnDestroy();

		mComponents.erase(
			std::remove_if(mComponents.begin(), mComponents.end(),
				[component](const Scope<Component>& c) { return c.get() == component; }),
			mComponents.end());
	}

	void Entity::MoveComponent(int32 from, int32 to)
	{
		const int32 count = static_cast<int32>(mComponents.size());

		if (from < 0 || from >= count || to < 0 || to >= count || from == to)
			return;

		Scope<Component> moved = std::move(mComponents[from]);
		mComponents.erase(mComponents.begin() + from);
		mComponents.insert(mComponents.begin() + to, std::move(moved));
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
