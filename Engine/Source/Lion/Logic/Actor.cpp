#include "Engine.h"
#include "Actor.h"

namespace Lion
{
	Actor::Actor()
	{
		mTransform = MakeReference<Transform>();
	}
}
