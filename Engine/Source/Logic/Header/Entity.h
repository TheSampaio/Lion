#pragma once

namespace owl
{
	class Entity
	{
	public:
		float X, Y, Z;

	public:
		OWL_API Entity();
		OWL_API virtual ~Entity();

		// === MAIN methods ======

		// It is called once every frame to update the entity
		virtual void OWL_API OnUpdate() = 0;

		// It is called once every frame to draw the entity
		virtual void OWL_API OnDraw() = 0;

		// It is called once every frame to calculate the entity's collision
		virtual void OWL_API OnCollision() {};

		// Sums values at entity position
		virtual void OWL_API AddMovement(float X, float Y, float Z) { this->X += X, this->Y += Y, this->Z += Z; }

		// === SET methods ======

		// Sets the entity position
		virtual void OWL_API SetPosition(float X, float Y, float Z) { this->X = X, this->Y = Y, this->Z = Z; }
	};
}
