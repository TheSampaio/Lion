#include "Paddle.h"

using namespace Lion;

void Paddle::OnAwake()
{
	GetTransform()->SetPosition(Vector(0.0f, -278.0f, Depth::Front));
	mRenderer = AddComponent<SpriteRenderer>("Resource/Sprite/Brickout/player.png");
}

void Paddle::OnUpdate()
{
	const auto& transform = GetTransform();
	const float32 maxWidth = Window::GetSize().width / 2.0f;
	const float32 halfSprite = mRenderer->GetSize().width / 2.0f;

	if (Input::GetKeyPress(KeyCode::D))
		transform->Translate(Vector(1.0f, 0.0f, 0.0f) * mSpeed * Clock::GetDeltaTime());

	else if (Input::GetKeyPress(KeyCode::A))
		transform->Translate(Vector(-1.0f, 0.0f, 0.0f) * mSpeed * Clock::GetDeltaTime());

	// Keep the paddle inside the window client area.
	const Vector position = transform->GetPosition();

	if (position.x + halfSprite >= maxWidth)
		transform->SetPosition(Vector(maxWidth - halfSprite, position.y, position.z));

	else if (position.x - halfSprite <= -maxWidth)
		transform->SetPosition(Vector(-maxWidth + halfSprite, position.y, position.z));
}
