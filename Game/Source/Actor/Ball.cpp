#include "Ball.h"

using namespace Lion;

void Ball::OnAwake()
{
	GetTransform()->SetPosition(Vector(0.0f, -260.0f, Depth::Middle));
	mRenderer = AddComponent<SpriteRenderer>("Resource/Sprite/Brickout/ball.png");
}

void Ball::OnUpdate()
{
	const auto& field = Window::GetSize();
	const auto& transform = GetTransform();
	const Size halfSprite = mRenderer->GetSize() * 0.5f;

	transform->Translate(Vector(mSpeed.x, mSpeed.y) * Clock::GetDeltaTime());

	const Vector position = transform->GetPosition();
	const float32 halfWidth = field.width / 2.0f;
	const float32 halfHeight = field.height / 2.0f;

	if (position.x >= halfWidth)
	{
		transform->SetPosition(Vector(halfWidth - halfSprite.width, position.y, position.z));
		mSpeed.x *= -1.0f;
	}
	else if (position.x <= -halfWidth)
	{
		transform->SetPosition(Vector(-halfWidth + halfSprite.width, position.y, position.z));
		mSpeed.x *= -1.0f;
	}

	if (position.y >= halfHeight)
	{
		transform->SetPosition(Vector(position.x, halfHeight - halfSprite.height, position.z));
		mSpeed.y *= -1.0f;
	}
	else if (position.y <= -halfHeight)
	{
		transform->SetPosition(Vector(position.x, -halfHeight + halfSprite.height, position.z));
		mSpeed.y *= -1.0f;
	}
}
