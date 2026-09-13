#include "Brick.h"
#include "GameRules.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void Brick::OnCollision(Entity& other)
{
	if (!GetOwner().IsEnabled())
		return;

	GameRules::RegisterBrickHit();
	GetOwner().SetVisible(false);
	GetOwner().SetEnabled(false);
}

LION_REGISTER_COMPONENT(Brick)
