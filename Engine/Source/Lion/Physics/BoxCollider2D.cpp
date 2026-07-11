#include "Engine.h"
#include "BoxCollider2D.h"

#include <box2d/box2d.h>

#include <Lion/Core/Log.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Serializer.h>
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

		// The collider size is expressed in unscaled pixels, like the sprite: the owner's world scale
		// (including any inherited from a parent) is applied on top, so the hitbox follows the entity.
		const Vector scale = GetOwner().GetWorldScale();
		const float32 halfWidth = (mWidth * 0.5f * std::fabs(scale.x)) * PhysicsWorld::MetersPerPixel;
		const float32 halfHeight = (mHeight * 0.5f * std::fabs(scale.y)) * PhysicsWorld::MetersPerPixel;
		const b2Polygon box = b2MakeBox(halfWidth, halfHeight);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = mDensity;
		shapeDef.material.friction = mFriction;
		shapeDef.material.restitution = mRestitution;
		shapeDef.enableContactEvents = true;  // Required for Actor::OnCollision callbacks.

		// EnsureBody so the shape attaches correctly regardless of component order.
		mShapeId = b2CreatePolygonShape(body->EnsureBody(), &shapeDef, &box);
	}

	void BoxCollider2D::Serialize(Serializer& serializer) const
	{
		serializer.Write("width", mWidth);
		serializer.Write("height", mHeight);
		serializer.Write("density", mDensity);
		serializer.Write("friction", mFriction);
		serializer.Write("restitution", mRestitution);
	}

	void BoxCollider2D::Deserialize(const Serializer& serializer)
	{
		mWidth = serializer.ReadFloat("width", 1.0f);
		mHeight = serializer.ReadFloat("height", 1.0f);
		mDensity = serializer.ReadFloat("density", 1.0f);
		mFriction = serializer.ReadFloat("friction", 0.2f);
		mRestitution = serializer.ReadFloat("restitution", 0.0f);
	}

	LION_REGISTER_COMPONENT(BoxCollider2D)
}
