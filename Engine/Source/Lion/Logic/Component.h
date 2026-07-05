#pragma once

namespace Lion
{
	class Entity;
	class Transform;

	// Base class for every behaviour or piece of data attached to an Entity.
	//
	// Components follow a Unity-like model: they are owned by a single Entity and receive
	// lifecycle callbacks driven by the Scene. Override the On* hooks to implement behaviour.
	// A component never outlives its owner and always has access to the owner's Transform.
	class Component
	{
	public:
		LION_API Component() = default;
		virtual LION_API ~Component() = default;

		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;

		// Returns the Entity this component is attached to.
		LION_API Entity& GetOwner() const { return *mOwner; }

		// Convenience accessor for the owner's Transform.
		LION_API Reference<Transform> GetTransform() const;

		// Whether lifecycle callbacks are dispatched to this component.
		LION_API bool IsEnabled() const { return mEnabled; }
		LION_API void SetEnabled(bool enabled) { mEnabled = enabled; }

		// Called once, right after the component is attached to its owner.
		virtual void OnAttach() {}

		// Called once, when the owner enters the scene.
		virtual void OnAwake() {}

		// Called every frame, before the main update pass.
		virtual void OnUpdateBegin() {}

		// Called every frame, during the main update pass.
		virtual void OnUpdate() {}

		// Called every frame, after the main update pass.
		virtual void OnUpdateEnd() {}

		// Called every frame, during the render pass.
		virtual void OnRender() {}

		// Called once, when the owner leaves the scene.
		virtual void OnDestroy() {}

		friend Entity;

	private:
		Entity* mOwner = nullptr;
		bool mEnabled = true;
	};
}
