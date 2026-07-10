#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Transform.h>

namespace Lion
{
	class Scene;

	// Base game object of the engine.
	//
	// Following a Unity-like model, every Entity always owns a Transform and can host any
	// number of Components. Derive from Entity (or Actor) to add scripted behaviour through
	// the On* hooks, and attach reusable behaviour or data through Components.
	class Entity
	{
	public:
		LION_API Entity();
		virtual LION_API ~Entity() = default;

		Entity(const Entity&) = delete;
		Entity& operator=(const Entity&) = delete;

		// Process-unique runtime id (not serialized). Used by the editor for pixel-perfect picking:
		// the id is written into the framebuffer's entity-id attachment and read back on click.
		LION_API int32 GetId() const { return mId; }

		// The entity's display name (shown in the editor Hierarchy and saved to scenes).
		LION_API const std::string& GetName() const { return mName; }
		LION_API void SetName(const std::string& name) { mName = name; }

		// Returns the scene that currently owns this entity (null while detached).
		LION_API Reference<Scene> GetScene() const { return mScene; }

		// Returns the entity's Transform, always present and never null. Its position/rotation/scale
		// are LOCAL to the parent; use the world accessors below for the composed transform.
		LION_API Reference<Transform> GetTransform() const { return mTransform; }

		// Parent/child hierarchy. Children inherit their parent's transform.
		LION_API Entity* GetParent() const { return mParent; }
		LION_API const std::vector<Entity*>& GetChildren() const { return mChildren; }

		// Reparents this entity. By default the world transform is preserved (the local one is
		// rebased); pass keepWorldTransform = false to link the parent and leave the local transform
		// untouched, as deserialization does. Ignored when it would create a cycle.
		LION_API void SetParent(Entity* parent, bool keepWorldTransform = true);

		// True when 'other' is somewhere up this entity's parent chain.
		LION_API bool IsDescendantOf(const Entity* other) const;

		// World-space transform, composed through the parent chain (rotation in degrees, around z).
		LION_API Vector GetWorldPosition() const;
		LION_API float32 GetWorldRotation() const;
		LION_API Vector GetWorldScale() const;

		// Assign a world-space transform; the local transform is derived from the parent's.
		LION_API void SetWorldPosition(const Vector& position);
		LION_API void SetWorldRotation(float32 degrees);
		LION_API void SetWorldScale(const Vector& scale);

		// Requests removal of this entity from its scene (deferred to the end of the frame).
		LION_API void RemoveFromScene();

		// Creates a component of type T, attaches it to this entity and returns it.
		template<typename T, typename... Args>
		T* AddComponent(Args&&... args)
		{
			static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

			auto component = MakeScope<T>(std::forward<Args>(args)...);
			T* raw = component.get();
			RegisterComponent(std::move(component), std::type_index(typeid(T)));
			return raw;
		}

		// Returns the attached component of type T, or nullptr when absent.
		template<typename T>
		T* GetComponent() const
		{
			return static_cast<T*>(FindComponent(std::type_index(typeid(T))));
		}

		// Returns whether a component of type T is attached.
		template<typename T>
		bool HasComponent() const
		{
			return FindComponent(std::type_index(typeid(T))) != nullptr;
		}

		// Removes the attached component of type T, if any.
		template<typename T>
		void RemoveComponent()
		{
			UnregisterComponent(std::type_index(typeid(T)));
		}

		// Attached components in their stored order (i.e. the order they were added). The editor
		// draws them in this order and can reorder them.
		LION_API const std::vector<Scope<Component>>& GetComponents() const { return mComponents; }

		// Removes a specific component instance (used by the editor's per-component remove button).
		LION_API void RemoveComponent(Component* component);

		// Moves the component at 'from' to index 'to', shifting the others (editor drag-to-reorder).
		LION_API void MoveComponent(int32 from, int32 to);

		// Override points for scripted behaviour of the entity itself.
		virtual void OnAwake() {}
		virtual void OnDestroy() {}

		virtual void OnUpdateBegin() {}
		virtual void OnUpdate() {}
		virtual void OnUpdateEnd() {}

		virtual void OnRender() {}

		friend Scene;

	private:
		const int32 mId;
		std::string mName = "Entity";
		Reference<Transform> mTransform;
		Reference<Scene> mScene;

		// Non-owning: the Scene owns every entity, so a parent never outlives its children's removal.
		Entity* mParent = nullptr;
		std::vector<Entity*> mChildren;

		std::vector<Scope<Component>> mComponents;
		std::unordered_map<std::type_index, Component*> mComponentLookup;

		// Unlinks this entity from its parent and orphans its children, without touching transforms.
		// Used by the Scene when an entity is destroyed, so no dangling pointers remain.
		void DetachFromHierarchy();

		// Component storage helpers (kept out of line to keep the templates thin).
		LION_API void RegisterComponent(Scope<Component> component, std::type_index type);
		LION_API Component* FindComponent(std::type_index type) const;
		LION_API void UnregisterComponent(std::type_index type);

		// Lifecycle dispatchers invoked by the Scene: run the entity hook, then its components.
		void Awake();
		void Destroy();
		void UpdateBegin();
		void Update();
		void UpdateEnd();
		void Render();
	};
}
