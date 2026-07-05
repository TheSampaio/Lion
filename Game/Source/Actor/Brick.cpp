#include "Brick.h"

using namespace Lion;

Brick::Brick(Reference<Texture> texture, const Vector position)
{
	GetTransform()->SetPosition(position);
	AddComponent<SpriteRenderer>(texture);
}
