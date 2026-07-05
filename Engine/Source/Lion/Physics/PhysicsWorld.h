#pragma once

#include <box2d/id.h>

namespace Lion
{
	class RigidBody2D;

	// Owns and drives a Box2D simulation for a Scene.
	//
	// The engine works in pixels while Box2D works in meters, so a fixed pixels-per-meter scale
	// bridges the two. The world advances with a fixed time step (decoupled from the frame rate),
	// then synchronizes body transforms back to their entities and dispatches contact events.
	class PhysicsWorld
	{
	public:
		// Conversion between engine pixels and Box2D meters.
		static constexpr float32 PixelsPerMeter = 100.0f;
		static constexpr float32 MetersPerPixel = 1.0f / PixelsPerMeter;

		explicit LION_API PhysicsWorld(const glm::vec2& gravity = { 0.0f, -9.81f });
		LION_API ~PhysicsWorld();

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		// Sets the world gravity, in meters per second squared.
		LION_API void SetGravity(const glm::vec2& gravity);

		// Advances the simulation, syncs transforms and dispatches collisions.
		void Step(float32 deltaTime);

		// Native Box2D world handle, used by physics components to create bodies.
		b2WorldId GetWorldId() const { return mWorldId; }

		// Rigid bodies register themselves so the world can sync their transforms each step.
		void Register(RigidBody2D* body);
		void Unregister(RigidBody2D* body);

	private:
		b2WorldId mWorldId;
		float32 mAccumulator = 0.0f;
		std::vector<RigidBody2D*> mBodies;

		void DispatchCollisions();
	};
}
