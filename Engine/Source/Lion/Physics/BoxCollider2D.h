#pragma once

#include <Lion/Logic/Component.h>

#include <box2d/id.h>

namespace Lion
{
	// Rectangular collision shape attached to the owner's RigidBody2D.
	//
	// Requires a RigidBody2D on the same entity, attached before this collider (so the body
	// already exists when the shape is created). The size is given in pixels and converted to
	// meters internally. Contact events are enabled so actors receive OnCollision callbacks.
	class BoxCollider2D : public Component
	{
	public:
		LION_API BoxCollider2D(float32 widthPixels, float32 heightPixels,
			float32 density = 1.0f, float32 friction = 0.2f, float32 restitution = 0.0f);

		void OnAwake() override;

		// Configuration accessors (used by serialization).
		float32 GetWidth() const { return mWidth; }
		float32 GetHeight() const { return mHeight; }
		float32 GetDensity() const { return mDensity; }
		float32 GetFriction() const { return mFriction; }
		float32 GetRestitution() const { return mRestitution; }

	private:
		float32 mWidth;
		float32 mHeight;
		float32 mDensity;
		float32 mFriction;
		float32 mRestitution;

		b2ShapeId mShapeId{};
	};
}
