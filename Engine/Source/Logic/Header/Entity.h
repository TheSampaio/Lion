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

		virtual void OWL_API OnUpdate() = 0;
		virtual void OWL_API OnDraw() = 0;

		virtual void OWL_API AddMovement(float X, float Y, float Z) { this->X += X, this->Y += Y, this->Z += Z; }
		virtual void OWL_API SetPosition(float X, float Y) { this->X = X, this->Y = Y; }
		virtual void OWL_API SetPosition(float X, float Y, float Z) { this->X = X, this->Y = Y, this->Z = Z; }
	};
}
