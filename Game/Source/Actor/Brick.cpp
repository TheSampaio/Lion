#include "Brick.h"

using namespace Lion;

Brick::Brick(Reference<Texture> texture, const glm::vec3 position)
{
	mPosition = position;
	mSprite = MakeScope<Sprite>(texture);
}

void Brick::OnRender()
{
	mSprite->Draw(mPosition.x, mPosition.y, mPosition.z);
}
