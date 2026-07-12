#include "Brick.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void Brick::OnCollision(Entity& other)
{
	// Removal is deferred to the end of the frame, so the entity outlives the contact that killed it —
	// which is what makes it safe to ask for it from inside the callback.
	GetOwner().RemoveFromScene();
}

LION_REGISTER_COMPONENT(Brick)
