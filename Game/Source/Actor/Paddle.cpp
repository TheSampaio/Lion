#include "Paddle.h"

using namespace Lion;

void Paddle::OnAwake()
{
	GetTransform()->SetPosition(Vector(0.0f, -260.0f, Depth::Front));

	mRenderer = AddComponent<SpriteRenderer>("Resource/Sprite/Brickout/player.png");
	const Size size = mRenderer->GetSize();

	// Kinematic body: driven by input velocity, unaffected by the ball's impacts.
	mBody = AddComponent<RigidBody2D>(BodyType::Kinematic);
	AddComponent<BoxCollider2D>(size.width, size.height, 1.0f, 0.0f, 1.0f);
}

void Paddle::OnUpdate()
{
	float32 velocityX = 0.0f;

	if (Input::GetKeyPress(KeyCode::D) || Input::GetKeyPress(KeyCode::Right))
		velocityX = mSpeed;

	else if (Input::GetKeyPress(KeyCode::A) || Input::GetKeyPress(KeyCode::Left))
		velocityX = -mSpeed;

	mBody->SetLinearVelocity(glm::vec2(velocityX, 0.0f));

	// Keep the paddle inside the window client area.
	const float32 maxX = (Window::GetSize().width / 2.0f) - (mRenderer->GetSize().width / 2.0f);
	const Vector position = GetTransform()->GetPosition();

	if (position.x > maxX)
	{
		mBody->SetPosition(glm::vec2(maxX, position.y));
		mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	}
	else if (position.x < -maxX)
	{
		mBody->SetPosition(glm::vec2(-maxX, position.y));
		mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
	}
}
