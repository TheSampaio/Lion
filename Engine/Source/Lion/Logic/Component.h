#pragma once

namespace Lion
{
	class Entity;
	class Reflector;
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

		// The components this one cannot work without, by their registered names. Attaching it attaches
		// those first, so a collider never lands on an entity that has no body to hang it on — the two are
		// co-dependent, and the editor should not be a place where you can build something that cannot run.
		//
		// Named rather than typed because the name is what crosses the module boundary: a component in the
		// game module can require one from the engine, and the other way round, without either knowing the
		// other's header. Use LION_REQUIRES to declare them.
		virtual std::vector<std::string> GetRequiredComponents() const { return {}; }

		// Describes this component's fields, once. The scene file saves them from this description and the
		// editor's Inspector draws them from it, so a field declared here needs saying nowhere else.
		virtual void Reflect(Reflector&) {}

		// Persists and restores this component's fields through an abstract archive.
		//
		// The default walks Reflect, which is why a component that describes its fields gets save/load
		// for nothing. The built-in components override these instead, because what they store is not
		// what they hold — a body type is an enum on the way in and a number on the way out — and a
		// description that lied about the field would be worse than no description at all.
		LION_API virtual void Serialize(Serializer& serializer) const;
		LION_API virtual void Deserialize(const Serializer& serializer);

		friend Entity;

	private:
		Entity* mOwner = nullptr;
		bool mEnabled = true;
		std::string mTypeName;  // Set by Entity from the ComponentRegistry when the component is attached.
	};
}

// Declares the components a component cannot work without, inside its class body:
//
//     class BoxCollider2D : public Component
//     {
//     public:
//         LION_REQUIRES("RigidBody2D");
//     };
//
// Attaching it attaches them first, if the entity does not have them already.
#define LION_REQUIRES(...)                                                                          \
	std::vector<std::string> GetRequiredComponents() const override { return { __VA_ARGS__ }; }
