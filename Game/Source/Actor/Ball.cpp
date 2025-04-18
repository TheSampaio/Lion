#include "Ball.h"

using namespace Lion;

void Ball::OnAwake()
{
	GetTransform()->SetPosition(Vector(0.0f, -260.0f, Depth::Middle));
	mSprite = MakeScope<Sprite>("Resource/Sprite/Brickout/ball.png");
}

void Ball::OnUpdate()
{
	const auto& field = Window::GetSize();
	const auto& transform = GetTransform();

	transform->Translate(Vector(mSpeed.x, mSpeed.y) * Clock::GetDeltaTime());

	if (transform->GetPosition().GetX() >= (field[0] / 2.0f))
	{
		transform->SetPosition(Vector((field[0] / 2.0f) - (mSprite->GetSize()[0] / 2.0f), transform->GetPosition().GetY(), transform->GetPosition().GetZ()));
		mSpeed.x *= -1.0f;
	}

	else if (transform->GetPosition().GetX() <= -(field[0] / 2.0f))
	{
		transform->SetPosition(Vector(-(field[0] / 2.0f) + (mSprite->GetSize()[0] / 2.0f), transform->GetPosition().GetY(), transform->GetPosition().GetZ()));
		mSpeed.x *= -1.0f;
	}

	if (transform->GetPosition().GetY() >=  (field[1] / 2.0f))
	{
		transform->SetPosition(Vector(transform->GetPosition().GetX(), (field[1] / 2.0f) - (mSprite->GetSize()[1] / 2.0f), transform->GetPosition().GetZ()));
		mSpeed.y *= -1.0f;
	}

	else if (transform->GetPosition().GetY() <= -(field[1] / 2.0f))
	{
		transform->SetPosition(Vector(transform->GetPosition().GetX(), -(field[1] / 2.0f) + (mSprite->GetSize()[1] / 2.0f), transform->GetPosition().GetZ()));
		mSpeed.y *= -1.0f;
	}
}

void Ball::OnRender()
{
	mSprite->Draw(GetTransform());
}
