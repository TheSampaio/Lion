#include "Brick.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void Brick::OnCollision(Entity& other)
{
	GetOwner().SetVisible(false);
	GetOwner().SetEnabled(false);
}

LION_REGISTER_COMPONENT(Brick)
