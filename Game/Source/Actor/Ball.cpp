#include "Ball.h"

using namespace Lion;

void Ball::OnAwake()
{
	mTransform = GetTransform();
	GetTransform()->SetPosition(Vector(0.0f, -260.0f, Depth::Middle));
	mSprite = MakeScope<Sprite>("Resource/Sprite/Brickout/ball.png");
}

void Ball::OnUpdate()
{
	const auto& field = Window::GetSize();

	mTransform->Translate(Vector(mSpeed.x, mSpeed.y) * Clock::GetDeltaTime());

	if (mTransform->GetPosition().x >= (field[0] / 2.0f))
	{
		mTransform->SetPosition(Vector((field[0] / 2.0f) - (mSprite->GetSize()[0] / 2.0f), mTransform->GetPosition().y, mTransform->GetPosition().z));
		mSpeed.x *= -1.0f;
	}

	else if (mTransform->GetPosition().x <= -(field[0] / 2.0f))
	{
		mTransform->SetPosition(Vector(-(field[0] / 2.0f) + (mSprite->GetSize()[0] / 2.0f), mTransform->GetPosition().y, mTransform->GetPosition().z));
		mSpeed.x *= -1.0f;
	}

	if (mTransform->GetPosition().y >=  (field[1] / 2.0f))
	{
		mTransform->SetPosition(Vector(mTransform->GetPosition().x, (field[1] / 2.0f) - (mSprite->GetSize()[1] / 2.0f), mTransform->GetPosition().z));
		mSpeed.y *= -1.0f;
	}

	else if (mTransform->GetPosition().y <= -(field[1] / 2.0f))
	{
		mTransform->SetPosition(Vector(mTransform->GetPosition().x, -(field[1] / 2.0f) + (mSprite->GetSize()[1] / 2.0f), mTransform->GetPosition().z));
		mSpeed.y *= -1.0f;
	}
}

void Ball::OnRender()
{
	mSprite->Draw(mTransform);
}
