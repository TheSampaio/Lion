#include "Paddle.h"

using namespace Lion;

void Paddle::OnAwake()
{
	GetTransform()->SetPosition(Vector(0.0f, -278.0f, Depth::Front));
	mSprite = MakeScope<Sprite>("Resource/Sprite/Brickout/player.png");
}

void Paddle::OnUpdate()
{
	const auto& field = Window::GetSize();
	const auto& transform = GetTransform();
	const float32 maxWidth = (field[0] / 2.0f);
	const float32 speed = 500.0f;

	if (Input::GetKeyPress(KeyCode::D))
		transform->Translate(Vector(1.0f, 0.0f, 0.0f) * speed * Clock::GetDeltaTime());

	else if (Input::GetKeyPress(KeyCode::A))
		transform->Translate(-Vector(1.0f, 0.0f, 0.0f) * speed * Clock::GetDeltaTime());

	// Limits player position to the window client's area
	if (transform->GetPosition().x + (mSprite->GetSize()[0] / 2.0f) >= maxWidth)
	{
		transform->SetPosition(
			Vector(
				maxWidth - (mSprite->GetSize()[0] / 2),
				transform->GetPosition().y,
				transform->GetPosition().z
			)
		);
	}

	else if (transform->GetPosition().x - (mSprite->GetSize()[0] / 2.0f) <= -maxWidth)
	{
		transform->SetPosition(
			Vector(
				-maxWidth + (mSprite->GetSize()[0] / 2),
				transform->GetPosition().y,
				transform->GetPosition().z
			)
		);
	}
}

void Paddle::OnRender()
{
	mSprite->Draw(GetTransform());
}
