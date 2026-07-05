#pragma once

#include <Lion/Logic/Entity.h>

namespace Lion
{
	// An Entity that participates in physics and collision resolution.
	//
	// Actor extends Entity with a collision callback. Use it for gameplay objects that need
	// to react to contacts (players, projectiles, obstacles, ...).
	class Actor : public Entity
	{
	public:
		LION_API Actor() = default;
		virtual LION_API ~Actor() = default;

		// Called when this actor starts touching another actor.
		virtual void OnCollision(Actor& other) {}
	};
}
