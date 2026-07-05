#include "Engine.h"
#include "Component.h"

#include <Lion/Logic/Entity.h>

namespace Lion
{
	Reference<Transform> Component::GetTransform() const
	{
		return mOwner->GetTransform();
	}
}
