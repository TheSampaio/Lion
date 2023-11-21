#pragma once

namespace Lion
{
	class Entity
	{
	public:
		float X, Y, Z;

	public:
		LION_API Entity();
		LION_API virtual ~Entity();

		// === MAIN methods ======

		// It is called once every frame to update the entity
		virtual void LION_API OnUpdate() = 0;

		// It is called once every frame to draw the entity
		virtual void LION_API OnDraw() = 0;

		// It is called once every frame to calculate the entity's collision
		virtual void LION_API OnCollision() {};

		// Sums values at entity position
		virtual void LION_API AddMovement(float X, float Y, float Z) { this->X += X, this->Y += Y, this->Z += Z; }

		// === SET methods ======

		// Sets the entity position
		virtual void LION_API SetPosition(float X, float Y, float Z) { this->X = X, this->Y = Y, this->Z = Z; }
	};
}
