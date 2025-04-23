#pragma once

#include <Lion/Logic/Entity.h>
#include <Lion/Math/Transform.h>

namespace Lion
{
	class Transform;

	class Actor : public Entity
	{
	public:
		LION_API Actor();
		virtual LION_API ~Actor() = default;

		virtual LION_API Reference<Transform> GetTransform() const { return mTransform; }

		virtual void OnCollision(Actor& other) {};

	private:
		Reference<Transform> mTransform;
	};
}
