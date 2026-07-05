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

		// Returns the scene that currently owns this entity (null while detached).
		LION_API Reference<Scene> GetScene() const { return mScene; }

		// Returns the entity's Transform, always present and never null.
		LION_API Reference<Transform> GetTransform() const { return mTransform; }

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

		// Override points for scripted behaviour of the entity itself.
		virtual void OnAwake() {}
		virtual void OnDestroy() {}

		virtual void OnUpdateBegin() {}
		virtual void OnUpdate() {}
		virtual void OnUpdateEnd() {}

		virtual void OnRender() {}

		friend Scene;

	private:
		Reference<Transform> mTransform;
		Reference<Scene> mScene;

		std::vector<Scope<Component>> mComponents;
		std::unordered_map<std::type_index, Component*> mComponentLookup;

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
