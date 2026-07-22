#include "Engine.h"
#include "Transform.h"

namespace Lion {

	Transform::Transform()
		: mPosition(0.0f), mRotation(0.0f), mScale(1.0f)
	{
	}

	Transform::Transform(const Vector2& position, float32 rotation, const Vector2& scale)
		: mPosition(position), mRotation(rotation), mScale(scale)
	{
	}

	Vector2 Transform::GetPosition() const
	{
		return mPosition;
	}

	float32 Transform::GetRotation() const
	{
		return mRotation;
	}

	Vector2 Transform::GetScale() const
	{
		return mScale;
	}

	void Transform::SetPosition(const Vector2& position)
	{
		mPosition = position;
	}

	void Transform::SetRotation(float32 rotation)
	{
		mRotation = rotation;
	}

	void Transform::SetScale(const Vector2& scale)
	{
		mScale = scale;
	}

	void Transform::Translate(const Vector2& delta)
	{
		mPosition = mPosition + delta;
	}

	void Transform::Rotate(float32 degrees)
	{
		mRotation = mRotation + degrees;
	}

	void Transform::Scale(const Vector2& factor)
	{
		mScale = mScale * factor;
	}
}
