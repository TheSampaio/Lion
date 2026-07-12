#pragma once

#include <Lion/Logic/Component.h>

#include <box2d/id.h>

namespace Lion
{
	class PhysicsWorld;

	// Simulation type of a rigid body, mirroring Box2D's body types.
	enum class BodyType
	{
		Static,     // Never moves (walls, ground, bricks).
		Kinematic,  // Moved by the user through velocity; not affected by forces (paddles).
		Dynamic,    // Fully simulated: moved by forces, gravity and collisions (the ball).
	};

	// Gives an Entity a physical presence in the Scene's PhysicsWorld.
	//
	// The rigid body creates a Box2D body positioned at its owner's Transform and keeps the two
	// in sync every physics step. Attach a BoxCollider2D to give it a collision shape. Physics
	// components must be attached while the entity is in a scene (typically inside OnAwake).
	class RigidBody2D : public Component
	{
	public:
		LION_API explicit RigidBody2D(BodyType type = BodyType::Dynamic, bool fixedRotation = false);

		void OnAwake() override;
		void OnDestroy() override;

		// A body follows its entity: switching the entity off takes the body out of the simulation, so a
		// disabled wall stops being a wall instead of quietly staying one.
		void OnEnable() override;
		void OnDisable() override;
		void Serialize(Serializer& serializer) const override;
		void Deserialize(const Serializer& serializer) override;

		// Linear velocity, expressed in pixels per second.
		LION_API void SetLinearVelocity(const glm::vec2& pixelsPerSecond);
		LION_API glm::vec2 GetLinearVelocity() const;

		// Teleports the body to a world position, expressed in pixels.
		LION_API void SetPosition(const glm::vec2& pixels);

		// Native Box2D handle, used by colliders to attach shapes.
		b2BodyId GetBodyId() const { return mBodyId; }

		// Creates the Box2D body if it does not exist yet and returns its id. Called by OnAwake and by
		// colliders, so the body is created regardless of component awake order.
		b2BodyId EnsureBody();

		// Configuration accessors (used by serialization).
		BodyType GetBodyType() const { return mType; }
		bool IsFixedRotation() const { return mFixedRotation; }

		// Configuration mutators (used by the editor Inspector). These update the stored settings;
		// they take effect when the body is created (the editor does not run the simulation).
		void SetBodyType(BodyType type) { mType = type; }
		void SetFixedRotation(bool value) { mFixedRotation = value; }

		// Copies the simulated transform back to the owner entity (called by PhysicsWorld).
		void SyncTransform();

	private:
		BodyType mType;
		bool mFixedRotation;

		b2BodyId mBodyId{};
		bool mHasBody = false;
		glm::vec2 mPendingVelocity{ 0.0f, 0.0f };  // Applied once the body is created.

		PhysicsWorld* mWorld = nullptr;
	};
}
