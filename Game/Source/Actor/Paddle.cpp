#include "Paddle.h"

using namespace Lion;

void Paddle::OnAwake()
{
	mTransform = GetTransform();
	mTransform->SetPosition(Vector(0.0f, -278.0f, Depth::Front));
	mSprite = MakeScope<Sprite>("Resource/Sprite/Brickout/player.png");
}

void Paddle::OnUpdate()
{
	const auto& field = Window::GetSize();
	const float32 maxWidth = (field[0] / 2.0f);

	if (Input::GetKeyPress(KeyCode::D))
		mTransform->Translate(Vector(1.0f, 0.0f, 0.0f) * mSpeed * Clock::GetDeltaTime());

	else if (Input::GetKeyPress(KeyCode::A))
		mTransform->Translate(-Vector(1.0f, 0.0f, 0.0f) * mSpeed * Clock::GetDeltaTime());

	// Limits player position to the window client's area
	if (mTransform->GetPosition().x + (mSprite->GetSize()[0] / 2.0f) >= maxWidth)
	{
		mTransform->SetPosition(
			Vector(
				maxWidth - (mSprite->GetSize()[0] / 2),
				mTransform->GetPosition().y,
				mTransform->GetPosition().z
			)
		);
	}

	else if (mTransform->GetPosition().x - (mSprite->GetSize()[0] / 2.0f) <= -maxWidth)
	{
		mTransform->SetPosition(
			Vector(
				-maxWidth + (mSprite->GetSize()[0] / 2),
				mTransform->GetPosition().y,
				mTransform->GetPosition().z
			)
		);
	}
}

void Paddle::OnRender()
{
	mSprite->Draw(mTransform);
}
