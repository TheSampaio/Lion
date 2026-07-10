#include "Engine.h"
#include "Script.h"

#include <Lion/Logic/Entity.h>

namespace Lion
{
	Reference<Transform> Script::GetTransform() const
	{
		return mOwner->GetTransform();
	}
}
