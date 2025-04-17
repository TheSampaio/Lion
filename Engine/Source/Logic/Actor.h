#pragma once

#include "Entity.h"

namespace Lion
{
	class Actor : public Entity
	{
	public:
		LION_API Actor() = default;
		virtual LION_API ~Actor() = default;

		virtual void OnCollision(Actor& other) {};
	};
}
