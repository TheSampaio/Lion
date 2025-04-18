#pragma once

#include "Vector.h"

namespace Lion
{
	class Renderer;
	class Sprite;

	class Transform
	{
	public:
		LION_API Transform();
		LION_API Transform(const Vector& position, const Vector& rotation, const Vector& scale);

		LION_API Vector GetPosition() const;
		LION_API Vector GetRotation() const;
		LION_API Vector GetScale() const;

		LION_API void SetPosition(const Vector& position);
		LION_API void SetRotation(const Vector& rotation);
		LION_API void SetScale(const Vector& scale);

		LION_API void Translate(const Vector& position);
		LION_API void Rotate(const Vector& rotation);
		LION_API void Scale(const Vector& scale);

		friend Sprite;

	private:
		Vector mPosition;
		Vector mRotation;
		Vector mScale;
	};
}
