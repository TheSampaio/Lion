#include "Paddle.h"

using namespace Lion;

void Paddle::OnAwake()
{
	mSprite = MakeScope<Sprite>("Resource/Sprite/Brickout/player.png");
}

void Paddle::OnRender()
{
	mSprite->Draw(0.0f, -278.0f, Depth::Front);
}
