#include "Paddle.h"

#include <Lion/Logic/ComponentRegistry.h>

using namespace Lion;

void Paddle::OnAwake()
{
	mBody = GetOwner().GetComponent<RigidBody2D>();
	mRenderer = GetOwner().GetComponent<SpriteRenderer>();
}

void Paddle::OnUpdate()
{
	float32 velocityX = 0.0f;

	if (Input::GetKeyPress(KeyCode::D) || Input::GetKeyPress(KeyCode::Right))
		velocityX = kSpeed;

	else if (Input::GetKeyPress(KeyCode::A) || Input::GetKeyPress(KeyCode::Left))
		velocityX = -kSpeed;

	mBody->SetLinearVelocity(glm::vec2(velocityX, 0.0f));

	// Keep the paddle inside the window's client area.
	const float32 maxX = (Window::GetSize().width / 2.0f) - GetHalfWidth();
	const Vector position = GetTransform()->GetPosition();

	if (std::abs(position.x) <= maxX)
		return;

	mBody->SetPosition(glm::vec2((position.x > 0.0f) ? maxX : -maxX, position.y));
	mBody->SetLinearVelocity(glm::vec2(0.0f, 0.0f));
}

float32 Paddle::GetHalfWidth() const
{
	return mRenderer->GetSize().width * 0.5f;
}

float32 Paddle::GetHalfHeight() const
{
	return mRenderer->GetSize().height * 0.5f;
}

LION_REGISTER_COMPONENT(Paddle)
