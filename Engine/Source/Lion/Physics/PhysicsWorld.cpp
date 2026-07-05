#include "Engine.h"
#include "PhysicsWorld.h"

#include <box2d/box2d.h>

#include <Lion/Logic/Actor.h>
#include <Lion/Physics/RigidBody2D.h>

namespace Lion
{
	// Simulate at a fixed rate, independent of the render frame rate.
	static constexpr float32 fixedTimeStep = 1.0f / 60.0f;
	static constexpr int32 subStepCount = 4;
	static constexpr int32 maxStepsPerFrame = 5;  // Avoids the "spiral of death" on long frames.

	PhysicsWorld::PhysicsWorld(const glm::vec2& gravity)
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { gravity.x, gravity.y };

		mWorldId = b2CreateWorld(&worldDef);
	}

	PhysicsWorld::~PhysicsWorld()
	{
		b2DestroyWorld(mWorldId);
	}

	void PhysicsWorld::SetGravity(const glm::vec2& gravity)
	{
		b2World_SetGravity(mWorldId, { gravity.x, gravity.y });
	}

	void PhysicsWorld::Step(float32 deltaTime)
	{
		mAccumulator += deltaTime;

		int32 steps = 0;

		while (mAccumulator >= fixedTimeStep && steps < maxStepsPerFrame)
		{
			b2World_Step(mWorldId, fixedTimeStep, subStepCount);
			mAccumulator -= fixedTimeStep;
			steps++;

			DispatchCollisions();
		}

		// Drop leftover time when we hit the step cap so it cannot accumulate forever.
		if (steps == maxStepsPerFrame)
			mAccumulator = 0.0f;

		// Push the simulated body transforms back onto their entities.
		for (RigidBody2D* body : mBodies)
			body->SyncTransform();
	}

	void PhysicsWorld::Register(RigidBody2D* body)
	{
		mBodies.push_back(body);
	}

	void PhysicsWorld::Unregister(RigidBody2D* body)
	{
		mBodies.erase(std::remove(mBodies.begin(), mBodies.end(), body), mBodies.end());
	}

	void PhysicsWorld::DispatchCollisions()
	{
		const b2ContactEvents events = b2World_GetContactEvents(mWorldId);

		for (int32 i = 0; i < events.beginCount; ++i)
		{
			const b2ContactBeginTouchEvent& contact = events.beginEvents[i];

			const b2BodyId bodyA = b2Shape_GetBody(contact.shapeIdA);
			const b2BodyId bodyB = b2Shape_GetBody(contact.shapeIdB);

			// User data is the owning Entity; only actors receive collision callbacks.
			auto* entityA = static_cast<Entity*>(b2Body_GetUserData(bodyA));
			auto* entityB = static_cast<Entity*>(b2Body_GetUserData(bodyB));

			auto* actorA = dynamic_cast<Actor*>(entityA);
			auto* actorB = dynamic_cast<Actor*>(entityB);

			if (actorA && actorB)
			{
				actorA->OnCollision(*actorB);
				actorB->OnCollision(*actorA);
			}
		}
	}
}
