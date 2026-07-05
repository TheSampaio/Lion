#include "Engine.h"
#include "BoxCollider2D.h"

#include <box2d/box2d.h>

#include <Lion/Core/Log.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Physics/PhysicsWorld.h>
#include <Lion/Physics/RigidBody2D.h>

namespace Lion
{
	BoxCollider2D::BoxCollider2D(float32 widthPixels, float32 heightPixels,
		float32 density, float32 friction, float32 restitution)
		: mWidth(widthPixels), mHeight(heightPixels),
		mDensity(density), mFriction(friction), mRestitution(restitution)
	{
	}

	void BoxCollider2D::OnAwake()
	{
		RigidBody2D* body = GetOwner().GetComponent<RigidBody2D>();

		if (!body)
		{
			Log::Console(LogLevel::Error, "[BoxCollider2D] Requires a RigidBody2D on the same entity.");
			return;
		}

		const float32 halfWidth = (mWidth * 0.5f) * PhysicsWorld::MetersPerPixel;
		const float32 halfHeight = (mHeight * 0.5f) * PhysicsWorld::MetersPerPixel;
		const b2Polygon box = b2MakeBox(halfWidth, halfHeight);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = mDensity;
		shapeDef.material.friction = mFriction;
		shapeDef.material.restitution = mRestitution;
		shapeDef.enableContactEvents = true;  // Required for Actor::OnCollision callbacks.

		mShapeId = b2CreatePolygonShape(body->GetBodyId(), &shapeDef, &box);
	}
}
