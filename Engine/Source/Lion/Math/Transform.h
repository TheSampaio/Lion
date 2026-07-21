#pragma once

#include <Lion/Math/Vector2.h>

namespace Lion
{
	class Renderer;
	class Sprite;

	// Where an entity is, how it is turned, and how big it is — on a plane. Position and scale are 2D,
	// because a 2D engine has nowhere to put a third; rotation is a single angle in degrees, because a
	// rotation on a plane is one turn about one axis, not a vector whose X and Y meant nothing.
	class Transform
	{
	public:
		LION_API Transform();
		LION_API Transform(const Vector2& position, float32 rotation, const Vector2& scale);

		LION_API Vector2 GetPosition() const;
		LION_API float32 GetRotation() const;
		LION_API Vector2 GetScale() const;

		LION_API void SetPosition(const Vector2& position);
		LION_API void SetRotation(float32 rotation);
		LION_API void SetScale(const Vector2& scale);

		LION_API void Translate(const Vector2& delta);
		LION_API void Rotate(float32 degrees);
		LION_API void Scale(const Vector2& factor);

		friend Sprite;

	private:
		Vector2 mPosition;
		float32 mRotation;
		Vector2 mScale;
	};
}
