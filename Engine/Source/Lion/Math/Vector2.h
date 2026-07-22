#pragma once

#include <Lion/Math/Vector.h>

namespace Lion
{
	// A two-component vector, for the places a 2D engine only ever has two: a position on the plane, a
	// scale across it, a camera's offset. The three-component Vector stays for what genuinely has a
	// third — a colour, a raw triple — but a Z on a transform in a 2D engine was a field that did
	// nothing but ask to be set to zero.
	//
	// It converts to the three-component Vector on its own (Z becomes zero), so the many things that
	// still speak Vector — the sprite batch, the physics glue — take a Vector2 without a cast. Going the
	// other way drops a component, so it is spelled out: Vector2(someVector) is a deliberate flattening.
	struct Vector2
	{
		float32 x, y;

		LION_API Vector2() : x(0.0f), y(0.0f) {}
		LION_API Vector2(float32 value) : x(value), y(value) {}
		LION_API Vector2(float32 x, float32 y) : x(x), y(y) {}

		// Down from three components to two: deliberate, because it forgets the Z.
		LION_API explicit Vector2(const Vector& vector) : x(vector.x), y(vector.y) {}

		// Up to three components, with Z at zero: implicit, because nothing is lost and a great deal of
		// code still asks for a Vector.
		LION_API operator Vector() const { return Vector(x, y); }

		LION_API Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
		LION_API Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
		LION_API Vector2 operator*(float32 scalar) const { return Vector2(x * scalar, y * scalar); }
		LION_API Vector2 operator*(const Vector2& other) const { return Vector2(x * other.x, y * other.y); }

		LION_API bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
		LION_API bool operator!=(const Vector2& other) const { return !(*this == other); }
	};
}
