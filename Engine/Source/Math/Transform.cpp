#include "Engine.h"
#include "Transform.h"

namespace Lion {

	Transform::Transform()
		: mPosition(0.0f), mRotation(0.0f), mScale(1.0f)
	{
	}

	Transform::Transform(const Vector& position, const Vector& rotation, const Vector& scale)
		: mPosition(position), mRotation(rotation), mScale(scale)
	{
	}

	Vector Transform::GetPosition() const
	{
		return mPosition;
	}

	Vector Transform::GetRotation() const
	{
		return mRotation;
	}

	Vector Transform::GetScale() const
	{
		return mScale;
	}

	void Transform::SetPosition(const Vector& position)
	{
		mPosition = position;
	}

	void Transform::SetRotation(const Vector& rotation)
	{
		mRotation = rotation;
	}

	void Transform::SetScale(const Vector& scale)
	{
		mScale = scale;
	}

	void Transform::Translate(const Vector& position)
	{
		mPosition = mPosition + position;
	}

	void Transform::Rotate(const Vector& rotation)
	{
		mRotation = mRotation + rotation;
	}

	void Transform::Scale(const Vector& scale)
	{
		mScale = Vector(
			mScale.GetX() * scale.GetX(),
			mScale.GetY() * scale.GetY(),
			mScale.GetZ() * scale.GetZ()
		);
	}
}
