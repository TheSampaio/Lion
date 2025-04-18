#include "Brick.h"

using namespace Lion;

Brick::Brick(Reference<Texture> texture, const Vector position)
{
	GetTransform()->SetPosition(position);
	mSprite = MakeScope<Sprite>(texture);
}

void Brick::OnRender()
{
	mSprite->Draw(GetTransform());
}
