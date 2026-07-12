#include "Engine.h"
#include "Entity.h"

#include <Lion/Logic/ComponentRegistry.h>
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

	bool Entity::IsDescendantOf(const Entity* other) const
	{
		for (const Entity* ancestor = mParent; ancestor; ancestor = ancestor->mParent)
		{
			if (ancestor == other)
				return true;
		}

		return false;
	}

	void Entity::SetParent(Entity* parent, bool keepWorldTransform)
	{
		// Guard against cycles: an entity cannot parent itself or one of its own descendants.
		if (parent == this || mParent == parent || (parent && parent->IsDescendantOf(this)))
			return;

		// The entity keeps its place in the world; only the local transform is rebased.
		const Vector worldPosition = GetWorldPosition();
		const float32 worldRotation = GetWorldRotation();
		const Vector worldScale = GetWorldScale();

		if (mParent)
		{
			std::vector<Entity*>& siblings = mParent->mChildren;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
		}

		mParent = parent;

		if (mParent)
			mParent->mChildren.push_back(this);

		if (!keepWorldTransform)
			return;

		SetWorldPosition(worldPosition);
		SetWorldRotation(worldRotation);
		SetWorldScale(worldScale);
	}

	void Entity::DetachFromHierarchy()
	{
		if (mParent)
		{
			std::vector<Entity*>& siblings = mParent->mChildren;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
			mParent = nullptr;
		}

		for (Entity* child : mChildren)
			child->mParent = nullptr;

		mChildren.clear();
	}

	Vector Entity::GetWorldPosition() const
	{
		const Vector local = mTransform->GetPosition();

		if (!mParent)
			return local;

		const Vector parentPosition = mParent->GetWorldPosition();
		const Vector parentScale = mParent->GetWorldScale();
		const float32 parentRotation = glm::radians(mParent->GetWorldRotation());

		const float32 scaledX = local.x * parentScale.x;
		const float32 scaledY = local.y * parentScale.y;
		const float32 cosAngle = std::cos(parentRotation);
		const float32 sinAngle = std::sin(parentRotation);

		return Vector(
			parentPosition.x + scaledX * cosAngle - scaledY * sinAngle,
			parentPosition.y + scaledX * sinAngle + scaledY * cosAngle,
			parentPosition.z + local.z);
	}

	float32 Entity::GetWorldRotation() const
	{
		const float32 local = mTransform->GetRotation().z;
		return mParent ? mParent->GetWorldRotation() + local : local;
	}

	Vector Entity::GetWorldScale() const
	{
		const Vector local = mTransform->GetScale();

		if (!mParent)
			return local;

		const Vector parentScale = mParent->GetWorldScale();
		return Vector(parentScale.x * local.x, parentScale.y * local.y, parentScale.z * local.z);
	}

	void Entity::SetWorldPosition(const Vector& position)
	{
		if (!mParent)
		{
			mTransform->SetPosition(position);
			return;
		}

		const Vector parentPosition = mParent->GetWorldPosition();
		const Vector parentScale = mParent->GetWorldScale();
		const float32 parentRotation = glm::radians(mParent->GetWorldRotation());

		// Undo the parent's rotation, then its scale, to recover the local offset.
		const float32 deltaX = position.x - parentPosition.x;
		const float32 deltaY = position.y - parentPosition.y;
		const float32 cosAngle = std::cos(-parentRotation);
		const float32 sinAngle = std::sin(-parentRotation);

		const float32 rotatedX = deltaX * cosAngle - deltaY * sinAngle;
		const float32 rotatedY = deltaX * sinAngle + deltaY * cosAngle;

		mTransform->SetPosition(Vector(
			parentScale.x != 0.0f ? rotatedX / parentScale.x : rotatedX,
			parentScale.y != 0.0f ? rotatedY / parentScale.y : rotatedY,
			position.z - parentPosition.z));
	}

	void Entity::SetWorldRotation(float32 degrees)
	{
		const float32 parentRotation = mParent ? mParent->GetWorldRotation() : 0.0f;
		const Vector local = mTransform->GetRotation();
		mTransform->SetRotation(Vector(local.x, local.y, degrees - parentRotation));
	}

	void Entity::SetWorldScale(const Vector& scale)
	{
		if (!mParent)
		{
			mTransform->SetScale(scale);
			return;
		}

		const Vector parentScale = mParent->GetWorldScale();
		mTransform->SetScale(Vector(
			parentScale.x != 0.0f ? scale.x / parentScale.x : scale.x,
			parentScale.y != 0.0f ? scale.y / parentScale.y : scale.y,
			parentScale.z != 0.0f ? scale.z / parentScale.z : scale.z));
	}

	Component* Entity::AddComponentByName(const std::string& name)
	{
		Scope<Component> component = ComponentRegistry::Create(name);

		if (!component)
			return nullptr;

		Component* raw = component.get();
		RegisterComponent(std::move(component), std::type_index(typeid(*raw)));
		return raw;
	}

	bool Entity::HasComponentByName(const std::string& name) const
	{
		for (const auto& component : mComponents)
			if (component->GetTypeName() == name)
				return true;

		return false;
	}

	void Entity::RegisterComponent(Scope<Component> component, std::type_index type)
	{
		component->mOwner = this;
		component->mTypeName = ComponentRegistry::GetName(type);

		// What a component cannot work without goes on before it does. A collider asks its entity for a
		// body the moment it wakes, so an entity that had to be told twice — once for the trait, once for
		// what the trait runs on — is an entity you can build wrong, and the editor is where you would.
		//
		// A required component that requires something else lands here too, and one that is already
		// attached is not attached twice.
		for (const std::string& required : component->GetRequiredComponents())
			if (!HasComponentByName(required))
				AddComponentByName(required);

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

	bool Entity::IsActive() const
	{
		// Enabling is inherited: switching a parent off switches off everything under it, so an entity
		// runs only when nothing above it says otherwise.
		for (const Entity* entity = this; entity != nullptr; entity = entity->mParent)
			if (!entity->mEnabled)
				return false;

		return true;
	}

	void Entity::SetEnabled(bool value)
	{
		if (mEnabled == value)
			return;

		mEnabled = value;

		// Not running a component's callbacks stops it doing anything, but not being anything — a body
		// left in the physics world would still block whatever ran into it. This is where it lets go.
		for (const auto& component : mComponents)
		{
			if (value)
				component->OnEnable();
			else
				component->OnDisable();
		}
	}

	void Entity::Awake()
	{
		for (const auto& component : mComponents)
			component->OnAwake();
	}

	void Entity::Destroy()
	{
		for (const auto& component : mComponents)
			component->OnDestroy();
	}

	void Entity::UpdateBegin()
	{
		if (!IsActive())
			return;

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnUpdateBegin();
	}

	void Entity::Update()
	{
		if (!IsActive())
			return;

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnUpdate();
	}

	void Entity::UpdateEnd()
	{
		if (!IsActive())
			return;

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnUpdateEnd();
	}

	void Entity::Render()
	{
		// Hidden is not the same as off: an invisible entity still updates and still collides. It is only
		// here, at the one call that puts something on screen, that the two part ways.
		if (!IsActive() || !mVisible)
			return;

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnRender();
	}

	void Entity::Collide(Entity& other)
	{
		if (!IsActive())
			return;

		for (const auto& component : mComponents)
			if (component->IsEnabled())
				component->OnCollision(other);
	}
}
