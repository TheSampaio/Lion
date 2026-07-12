#pragma once

namespace Lion
{
	class Entity;
	class Serializer;
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

		// Registered type name (see LION_REGISTER_COMPONENT), assigned when the component is attached.
		// Empty for a type that was never registered, in which case serialization skips it.
		LION_API const std::string& GetTypeName() const { return mTypeName; }

		// Convenience accessor for the owner's Transform.
		LION_API Reference<Transform> GetTransform() const;

		// Whether lifecycle callbacks are dispatched to this component.
		LION_API bool IsEnabled() const { return mEnabled; }
		LION_API void SetEnabled(bool enabled) { mEnabled = enabled; }

		// Called once, right after the component is attached to its owner.
		virtual void OnAttach() {}

		// Called once, when the owner enters the scene.
		virtual void OnAwake() {}

		// Called when the owner is switched on or off. Skipping a component's callbacks is enough to stop
		// it *doing* anything, but not to stop it *being* anything: a rigid body left in the world would
		// still block whatever ran into it. A component that keeps state outside itself takes it back here.
		virtual void OnEnable() {}
		virtual void OnDisable() {}

		// Called every frame, before the main update pass.
		virtual void OnUpdateBegin() {}

		// Called every frame, during the main update pass.
		virtual void OnUpdate() {}

		// Called every frame, after the main update pass.
		virtual void OnUpdateEnd() {}

		// Called every frame, during the render pass.
		virtual void OnRender() {}

		// Called when the owner starts touching another entity. Both entities need a collider, and both
		// hear about it — each from its own side of the contact.
		//
		// This is where collision lives because this is where behaviour lives: an entity is the thing,
		// and a component is one trait of it. A trait that cares about being hit says so; the ones that
		// do not are not asked.
		virtual void OnCollision(Entity& other) {}

		// Called once, when the owner leaves the scene.
		virtual void OnDestroy() {}

		// Persists and restores this component's fields through an abstract archive. The base does
		// nothing; override to make a component's configuration round-trip through save/load. User
		// components override these the same way the built-in ones do.
		virtual void Serialize(Serializer&) const {}
		virtual void Deserialize(const Serializer&) {}

		friend Entity;

	private:
		Entity* mOwner = nullptr;
		bool mEnabled = true;
		std::string mTypeName;  // Set by Entity from the ComponentRegistry when the component is attached.
	};
}
