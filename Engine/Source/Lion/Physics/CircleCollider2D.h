#pragma once

#include <Lion/Logic/Component.h>

#include <box2d/id.h>

namespace Lion
{
	// Circular collision shape attached to the owner's RigidBody2D.
	//
	// Requires a RigidBody2D on the same entity, attached before this collider (so the body
	// already exists when the shape is created). The radius is given in pixels and converted to
	// meters internally. Contact events are enabled so actors receive OnCollision callbacks.
	class CircleCollider2D : public Component
	{
	public:
		LION_API CircleCollider2D(float32 radiusPixels,
			float32 density = 1.0f, float32 friction = 0.2f, float32 restitution = 0.0f);

		void OnAwake() override;

		// Configuration accessors (used by serialization).
		float32 GetRadius() const { return mRadius; }
		float32 GetDensity() const { return mDensity; }
		float32 GetFriction() const { return mFriction; }
		float32 GetRestitution() const { return mRestitution; }

		// Configuration mutators (used by the editor Inspector; applied when the shape is created).
		void SetRadius(float32 value) { mRadius = value; }
		void SetDensity(float32 value) { mDensity = value; }
		void SetFriction(float32 value) { mFriction = value; }
		void SetRestitution(float32 value) { mRestitution = value; }

	private:
		float32 mRadius;
		float32 mDensity;
		float32 mFriction;
		float32 mRestitution;

		b2ShapeId mShapeId{};
	};
}
