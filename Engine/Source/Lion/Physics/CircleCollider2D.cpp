#include "Engine.h"
#include "CircleCollider2D.h"

#include <box2d/box2d.h>

#include <Lion/Core/Log.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Physics/PhysicsWorld.h>
#include <Lion/Physics/RigidBody2D.h>

namespace Lion
{
	CircleCollider2D::CircleCollider2D(float32 radiusPixels,
		float32 density, float32 friction, float32 restitution)
		: mRadius(radiusPixels),
		mDensity(density), mFriction(friction), mRestitution(restitution)
	{
	}

	void CircleCollider2D::OnAwake()
	{
		RigidBody2D* body = GetOwner().GetComponent<RigidBody2D>();

		if (!body)
		{
			Log::Console(LogLevel::Error, "[CircleCollider2D] Requires a RigidBody2D on the same entity.");
			return;
		}

		b2Circle circle;
		circle.center = { 0.0f, 0.0f };
		circle.radius = mRadius * PhysicsWorld::MetersPerPixel;

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = mDensity;
		shapeDef.material.friction = mFriction;
		shapeDef.material.restitution = mRestitution;
		shapeDef.enableContactEvents = true;  // Required for Actor::OnCollision callbacks.

		mShapeId = b2CreateCircleShape(body->GetBodyId(), &shapeDef, &circle);
	}
}
