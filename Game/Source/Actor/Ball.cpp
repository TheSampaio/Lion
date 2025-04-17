#include "Ball.h"

using namespace Lion;

void Ball::OnAwake()
{
	mSprite = MakeScope<Sprite>("Resource/Sprite/Brickout/ball.png");
}

void Ball::OnRender()
{
	mSprite->Draw(0.0f, -260.0f, Depth::Upper);
}
